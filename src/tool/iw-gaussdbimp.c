/*
**    ▄▄▄▄  ▄▄▄  ▄▄▄▄                   
** ▀▀ ▀███  ███  ███▀            ▄▄     
** ██  ███  ███  ███ ▄███▄ ████▄ ██ ▄█▀ 
** ██  ███▄▄███▄▄███ ██ ██ ██ ▀▀ ████   
** ██▄  ▀████▀████▀  ▀███▀ ██    ██ ▀█▄ 
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <libpq-fe.h>

/* Dynamic buffer to hold a single streaming SQL statement */
typedef struct {
  char* data;
  size_t size;
  size_t cap;
} iw_buf;

static void iw_buf_init(iw_buf* b) {
  b->cap = 1024;
  b->size = 0;
  b->data = (char*)malloc(b->cap);
  if (!b->data) {
    fprintf(stderr, "Fatal: Out of memory\n");
    exit(EXIT_FAILURE);
  }
  b->data[0] = '\0';
}

static void iw_buf_add(iw_buf* b, char c) {
  if (b->size + 2 > b->cap) {
    b->cap *= 2;
    char* next = (char*)realloc(b->data, b->cap);
    if (!next) {
      fprintf(stderr, "Fatal: Out of memory\n");
      free(b->data);
      exit(EXIT_FAILURE);
    }
    b->data = next;
  }
  b->data[b->size++] = c;
  b->data[b->size] = '\0';
}

static void iw_buf_reset(iw_buf* b) {
  b->size = 0;
  if (b->data) {
    b->data[0] = '\0';
  }
}

static void iw_buf_free(iw_buf* b) {
  if (b->data) {
    free(b->data);
    b->data = NULL;
  }
  b->size = 0;
  b->cap = 0;
}

/* Trim trailing whitespace and ensure null-termination */
static void iw_buf_trim(iw_buf* b) {
  while (b->size > 0 && isspace((unsigned char)b->data[b->size - 1])) {
    b->size--;
  }
  b->data[b->size] = '\0';
}

/* Execute a single statement inside its own isolated transaction */
static int iw_exec_tx(PGconn* conn, const char* sql, size_t id) {
  if (!conn) {
    return 1; /* Dry-run / disconnected test mode */
  }

  PGresult* res = PQexec(conn, "BEGIN");
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr, "[Stmt %zu] Failed to BEGIN: %s", id, PQerrorMessage(conn));
    PQclear(res);
    return 0;
  }
  PQclear(res);

  res = PQexec(conn, sql);
  ExecStatusType status = PQresultStatus(res);
  if (status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK) {
    fprintf(stderr, "[Stmt %zu] Execution failed: %s", id, PQerrorMessage(conn));
    PQclear(res);

    PGresult* rb_res = PQexec(conn, "ROLLBACK");
    PQclear(rb_res);
    return 0;
  }
  PQclear(res);

  res = PQexec(conn, "COMMIT");
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    fprintf(stderr, "[Stmt %zu] Failed to COMMIT: %s", id, PQerrorMessage(conn));
    PQclear(res);
    return 0;
  }
  PQclear(res);

  printf("[Stmt %zu] Committed successfully.\n", id);
  return 1;
}

/* Stream-read file: parse statement-by-statement and execute immediately */
static void iw_exec_stream(PGconn* conn, FILE* fp) {
  iw_buf b;
  iw_buf_init(&b);

  int in_quote = 0;
  size_t count = 0;
  size_t succ = 0;
  size_t fail = 0;
  int c;

  while ((c = fgetc(fp)) != EOF) {
    /* Skip leading whitespace if buffer is empty */
    if (b.size == 0 && isspace((unsigned char)c)) {
      continue;
    }

    /* Handle SQL single-line comments (-- comment) */
    if (!in_quote && c == '-') {
      int next_c = fgetc(fp);
      if (next_c == '-') {
        while ((c = fgetc(fp)) != EOF && c != '\n');
        continue;
      } else if (next_c != EOF) {
        ungetc(next_c, fp);
      }
    }

    /* Handle single quotes and escaped quotes ('') */
    if (c == '\'') {
      iw_buf_add(&b, (char)c);
      if (in_quote) {
        int next_c = fgetc(fp);
        if (next_c == '\'') {
          iw_buf_add(&b, (char)next_c);
        } else {
          if (next_c != EOF) {
            ungetc(next_c, fp);
          }
          in_quote = 0;
        }
      } else {
        in_quote = 1;
      }
      continue;
    }

    /* Delimiter ';' reached outside quotes -> execute current statement */
    if (c == ';' && !in_quote) {
      iw_buf_trim(&b);
      if (b.size > 0) {
        count++;
        printf("Statement %zu: %s\n", count, b.data);
        if (iw_exec_tx(conn, b.data, count)) {
          succ++;
        } else {
          fail++;
        }
      }
      iw_buf_reset(&b);
      continue;
    }

    iw_buf_add(&b, (char)c);
  }

  /* Handle trailing statement without ending semicolon before EOF */
  iw_buf_trim(&b);
  if (b.size > 0) {
    count++;
    if (iw_exec_tx(conn, b.data, count)) {
      succ++;
    } else {
      fail++;
    }
  }

  iw_buf_free(&b);

  printf("\n--- Stream Execution Summary ---\n");
  printf("Total statements executed : %zu\n", count);
  printf("Successful                : %zu\n", succ);
  printf("Failed                    : %zu\n", fail);
}

int main(int argc, char** argv) {
  const char* filename = (argc > 1) ? argv[1] : "8673839872.sql";
  const char* conninfo = getenv("DB_CONNINFO");

  PGconn* conn = NULL;
  if (conninfo) {
    conn = PQconnectdb(conninfo);
    if (PQstatus(conn) != CONNECTION_OK) {
      fprintf(stderr, "Database connection failed: %s", PQerrorMessage(conn));
      PQfinish(conn);
      return EXIT_FAILURE;
    }
    printf("Connected to database successfully.\n");
  }

  FILE* fp = fopen(filename, "r");
  if (!fp) {
    perror("Failed to open SQL file");
    if (conn) {
      PQfinish(conn);
    }
    return EXIT_FAILURE;
  }

  iw_exec_stream(conn, fp);

  fclose(fp);
  if (conn) {
    PQfinish(conn);
  }
  return EXIT_SUCCESS;
}