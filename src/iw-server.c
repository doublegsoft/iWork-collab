/*
**    ▄▄▄▄  ▄▄▄  ▄▄▄▄                   
** ▀▀ ▀███  ███  ███▀            ▄▄     
** ██  ███  ███  ███ ▄███▄ ████▄ ██ ▄█▀ 
** ██  ███▄▄███▄▄███ ██ ██ ██ ▀▀ ████   
** ██▄  ▀████▀████▀  ▀███▀ ██    ██ ▀█▄ 
*/
#include <libwebsockets.h>
#include <uv.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>

#include "iw-pkt-codec.h"
#include "iw-ws.h"

static struct lws_context* context = NULL;
static int g_server_busy = 0;

#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

/*!
** Execute command with arguments and optionally redirect output to file.
**
** @param cmd        executable name (e.g. "ls")
** @param argv       argument array (NULL terminated)
** @param output     optional memory buffer (can be NULL)
** @param out_size   buffer size
** @param outfile    output file path (can be NULL)
**
** @return exit code (>=0), or -1 on error
*/
static int 
iw_exec_shell_capture(const char *cmd,
                      char* const argv[],
                      char* output,
                      size_t out_size,
                      const char* outfile)
{
  int pipefd[2];
  pid_t pid;
  ssize_t n;
  size_t total = 0;
  char buffer[256];
  int status = 0;
  int out_fd = -1;

  if (cmd == NULL || argv == NULL) {
    return -1;
  }

  if (pipe(pipefd) < 0) {
    return -1;
  }

  pid = fork();
  if (pid < 0) {
    close(pipefd[0]);
    close(pipefd[1]);
    return -1;
  }

  if (pid == 0) {
    // child
    close(pipefd[0]);

    // 如果指定了文件，则打开文件
    if (outfile != NULL) {
      out_fd = open(outfile, O_CREAT | O_WRONLY | O_TRUNC, 0644);
      if (out_fd < 0) {
        _exit(127);
      }

      dup2(out_fd, STDOUT_FILENO);
      dup2(out_fd, STDERR_FILENO);
      close(out_fd);
    } else {
      // 否则走 pipe
      dup2(pipefd[1], STDOUT_FILENO);
      dup2(pipefd[1], STDERR_FILENO);
    }

    close(pipefd[1]);

    execvp(cmd, argv);
    perror("execvp failed");
    _exit(127);
  }

  // parent
  close(pipefd[1]);

  // 如果没有输出 buffer，就不用读 pipe（避免阻塞仍建议读丢弃）
  while ((n = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
    if (output != NULL && out_size > 0) {
      size_t copy = (total + n < out_size - 1) ? n : (out_size - 1 - total);
      if (copy > 0) {
        memcpy(output + total, buffer, copy);
      }
    }
    total += n;
  }

  if (output != NULL && out_size > 0) {
    size_t end = (total < out_size - 1) ? total : (out_size - 1);
    output[end] = '\0';
  }

  close(pipefd[0]);

  if (waitpid(pid, &status, 0) < 0) {
    return -1;
  }

  if (WIFEXITED(status)) {
    return WEXITSTATUS(status);
  }

  return -1;
}

/*!
** Dumps a binary packet payload to a file for debugging or logging purposes.
**
** This function ensures that a target directory ("recv") exists, generates a 
** unique filename based on the current timestamp and an internal sequence 
** counter, and writes the provided byte array into that newly created file.
** 
** Generated filename format: recv/iw-pkt-YYYYMMDD-HHMMSS-<seq>.bin
**
** @param data Pointer to the binary packet data/payload to be saved.
** @param len  The length (in bytes) of the data buffer.
*/
static void
iw_dump_binary_packet_to_file(const unsigned char* data, size_t len)
{
  static unsigned long seq = 0;
  const char* dir = "recv";
  char path[256];
  FILE* f = NULL;
  time_t now = time(NULL);
  struct tm tm_now;

  if (data == NULL || len == 0) {
    return;
  }

  if (mkdir(dir, 0755) != 0 && errno != EEXIST) {
    fprintf(stderr, "[WS] mkdir(%s) failed: %s\n", dir, strerror(errno));
    return;
  }

  if (localtime_r(&now, &tm_now) == NULL) {
    memset(&tm_now, 0, sizeof(tm_now));
  }

  snprintf(path,
           sizeof(path),
           "%s/iw-pkt-%04d%02d%02d-%02d%02d%02d-%06lu.bin",
           dir,
           tm_now.tm_year + 1900,
           tm_now.tm_mon + 1,
           tm_now.tm_mday,
           tm_now.tm_hour,
           tm_now.tm_min,
           tm_now.tm_sec,
           ++seq);

  f = fopen(path, "wb");
  if (f == NULL) {
    fprintf(stderr, "[WS] fopen(%s) failed: %s\n", path, strerror(errno));
    return;
  }

  if (fwrite(data, 1, len, f) != len) {
    fprintf(stderr, "[WS] fwrite(%s) short write: %s\n", path, strerror(errno));
  } else {
    printf("[WS] dumped binary packet to %s (%zu bytes)\n", path, len);
  }
  fclose(f);
}

static void
iw_send_busy(struct lws* wsi, const void* in, size_t len)
{
  iw_prompt_p req = NULL;
  iw_prompt_p resp = NULL;
  unsigned char* encoded = NULL;
  size_t encoded_size = 0;
  const char* busy_text = "BUSY";

  if (wsi == NULL) {
    return;
  }

  if (in != NULL && len > 0) {
    req = iw_prompt_decode((const unsigned char*)in, len);
  }

  if (req != NULL) {
    resp = iw_prompt_init();
    if (resp != NULL) {
      resp->magic = req->magic;
      memcpy(resp->version, req->version, sizeof(resp->version));
      resp->request = req->request;
      memcpy(resp->type, req->type, sizeof(resp->type));
      resp->file_count = 0;
      resp->text_length = (int)strlen(busy_text);
      // resp->length = resp->text_length;
      resp->text = (char*)malloc((size_t)resp->text_length);
      if (resp->text != NULL) {
        memcpy(resp->text, busy_text, (size_t)resp->text_length);
        iw_prompt_encode(resp, &encoded, &encoded_size);
      }
    }
  }

  if (encoded != NULL && encoded_size > 0) {
    unsigned char* out = (unsigned char*)malloc(LWS_PRE + encoded_size);
    if (out != NULL) {
      memcpy(out + LWS_PRE, encoded, encoded_size);
      lws_write(wsi, out + LWS_PRE, encoded_size, LWS_WRITE_BINARY);
      free(out);
    }
  } else {
    unsigned char buf[LWS_PRE + 16];
    unsigned char* p = &buf[LWS_PRE];
    size_t n = strlen(busy_text);
    memcpy(p, busy_text, n);
    lws_write(wsi, p, n, LWS_WRITE_TEXT);
  }

  if (encoded != NULL) {
    free(encoded);
  }
  if (resp != NULL) {
    iw_prompt_free(resp);
  }
  if (req != NULL) {
    iw_prompt_free(req);
  }
}

/*!
** WebSocket request processing callback
**
** Handles lifecycle and message events from libwebsockets.
**
** @param wsi    WebSocket connection instance
** @param reason Callback reason (event type)
** @param user   Per-session user data (unused here)
** @param in     Input data buffer
** @param len    Length of input data
**
** @return int   0 on success
*/
static int 
iw_request_process(struct lws* wsi, enum lws_callback_reasons reason,
                   void* user, void* in, size_t len) {
  switch (reason) 
  {
    case LWS_CALLBACK_ESTABLISHED:
      printf("[WS] Connection established\n");
      break;

    case LWS_CALLBACK_RECEIVE: {
      unsigned char* encoded_reply = NULL;
      size_t encoded_reply_size = 0;
      printf("[WS] received request: %d bytes\n", (int)len);
      if (lws_frame_is_binary(wsi)) {
        iw_dump_binary_packet_to_file((const unsigned char*)in, len);
      }

      if (g_server_busy) {
        fprintf(stderr, "[WS] busy: reject new request\n");
        iw_send_busy(wsi, in, len);
        break;
      }

      int magic = 0;
      char type[2] = {0};
      memcpy((void*)&magic, in, 4);
      if (magic != 287454020) 
      {
        printf("[WS] invalid magic\n");
        return -1;
      }
      memcpy((void*)type, in + 4, 2);
      if (type[0] == 'P' && type[1] == 'T') {
        iw_prompt_p prompt = iw_prompt_decode((const unsigned char*)in, len);
        if (prompt == NULL) {
          fprintf(stderr, "[WS] Invalid prompt packet\n");
          lws_close_reason(wsi,
                           LWS_CLOSE_STATUS_INVALID_PAYLOAD,
                           (unsigned char*)"invalid prompt packet",
                           (size_t)strlen("invalid prompt packet"));
          return -1;
        }
        // 调用大语言模型
      } else if (type[0] == 'C' && type[1] == 'D') {
        iw_coding_p coding = iw_coding_decode((const unsigned char*)in, len);
        printf("[WS] coding: %ld text=%.*s\n",
               coding->request,
               coding->text_length,
               coding->text != NULL ? coding->text : "");
        if (coding == NULL) {
          fprintf(stderr, "[WS] Invalid coding packet\n");
          lws_close_reason(wsi,
                           LWS_CLOSE_STATUS_INVALID_PAYLOAD,
                           (unsigned char*)"invalid coding packet",
                           (size_t)strlen("invalid coding packet"));
          return -1;
        }
        iw_exec_shell_capture("ls", (char*[]){"ls", "-l", "./", NULL}, NULL, 0, "output.txt");

        iw_generation_p generation = iw_generation_init();
        iw_generation_set_magic(generation, 287454020);
        iw_generation_set_request(generation, coding->request);
        iw_generation_set_text_length(generation, 11);
        iw_generation_set_text(generation, "hello world", 11);
        
        iw_generation_encode(generation, &encoded_reply, &encoded_reply_size);
        unsigned char* buf = (unsigned char*)malloc(LWS_PRE + encoded_reply_size);
        memcpy(buf + LWS_PRE, encoded_reply, encoded_reply_size);
        free(encoded_reply);
        lws_write(wsi, buf + LWS_PRE, encoded_reply_size, LWS_WRITE_BINARY);
        free(buf);
      } else {
        fprintf(stderr, "[WS] invalid type: %c%c\n", type[0], type[1]);
        lws_close_reason(wsi,
                         LWS_CLOSE_STATUS_INVALID_PAYLOAD,
                         (unsigned char*)"invalid type",
                         (size_t)strlen("invalid type"));
        return -1;
      }
      iw_prompt_p response = NULL;
      char response_text[256];
      int response_text_len = 0;

      g_server_busy = 1;

      // if (generation != NULL) {
      //   printf("[WS] generation request=%ld text=%.*s\n",
      //          generation->request,
      //          generation->text_length,
      //          generation->text != NULL ? generation->text : "");

      //   handled = iw_send_generation_ack(wsi, generation);
      //   iw_generation_free(generation);
      //   g_server_busy = 0;

      //   if (handled != 0) {
      //     fprintf(stderr, "[WS] failed to send generation ACK\n");
      //     return -1;
      //   }
      //   break;
      // }

      // printf("[WS] prompt request=%ld text=%.*s\n",
      //        request->request,
      //        request->text_length,
      //        request->text != NULL ? request->text : "");

      // response = iw_prompt_init();
      // if (response == NULL) {
      //   iw_prompt_free(request);
      //   g_server_busy = 0;
      //   break;
      // }

      // response->magic = request->magic;
      // memcpy(response->version, request->version, sizeof(response->version));
      // response->request = request->request;
      // memcpy(response->type, request->type, sizeof(response->type));
      // response->file_count = 0;

      // response_text_len = snprintf(response_text,
      //                              sizeof(response_text),
      //                              "ACK:%.*s",
      //                              request->text_length,
      //                              request->text != NULL ? request->text : "");
      // if (response_text_len < 0) {
      //   iw_prompt_free(request);
      //   iw_prompt_free(response);
      //   g_server_busy = 0;
      //   break;
      // }
      // if ((size_t)response_text_len >= sizeof(response_text)) {
      //   response_text_len = (int)sizeof(response_text) - 1;
      // }

      // response->text_length = response_text_len;
      // response->text = (char*)malloc((size_t)response->text_length);
      // if (response->text == NULL) {
      //   iw_prompt_free(request);
      //   iw_prompt_free(response);
      //   g_server_busy = 0;
      //   break;
      // }
      // memcpy(response->text, response_text, (size_t)response->text_length);

      // iw_prompt_encode(response, &encoded_reply, &encoded_reply_size);
      // if (encoded_reply == NULL || encoded_reply_size == 0) {
      //   iw_prompt_free(request);
      //   iw_prompt_free(response);
      //   g_server_busy = 0;
      //   break;
      // }

      // {
      //   unsigned char* out = (unsigned char*)malloc(LWS_PRE + encoded_reply_size);
      //   if (out == NULL) {
      //     free(encoded_reply);
      //     iw_prompt_free(request);
      //     iw_prompt_free(response);
      //     g_server_busy = 0;
      //     break;
      //   }
      //   memcpy(out + LWS_PRE, encoded_reply, encoded_reply_size);
      //   lws_write(wsi, out + LWS_PRE, encoded_reply_size, LWS_WRITE_BINARY);
      //   free(out);
      // }

      // free(encoded_reply);
      // iw_prompt_free(request);
      // iw_prompt_free(response);
      g_server_busy = 0;
      break;
    }

    case LWS_CALLBACK_CLOSED:
      printf("[WS] Connection closed\n");
      break;

    case LWS_CALLBACK_WS_PEER_INITIATED_CLOSE:
      printf("[WS] Peer initiated close\n");
      return -1;

    default:
      break;
  }
  return 0;
}

/*!
** Signal handler for graceful server shutdown
**
** This callback is triggered when a registered OS signal
** (e.g., SIGINT / Ctrl+C) is received.
**
** @param handle  libuv signal handle
** @param signum  Signal number received
**
** Behavior:
** 1. Prints shutdown message
** 2. Stops the libuv event loop
*/
static void 
iw_server_single(uv_signal_t* handle, int signum) {
  printf("\nInterrupt received. Stopping...\n");
  uv_stop(handle->loop);
}

static struct lws_protocols protocols[] = {
  {
    iw_ws_protocol, // Protocol name (used by clients connecting)
    iw_request_process,   // Callback function
    0,                  // Per-session data size
    0,                  // Maximum frame size
  },
  { NULL, NULL, 0, 0 }  /* terminator */
};

int main(void) 
{
  struct lws_context_creation_info info;
  uv_loop_t* loop = uv_default_loop();
  
  // Setup graceful exit on CTRL+C
  uv_signal_t sigint;
  uv_signal_init(loop, &sigint);
  uv_signal_start(&sigint, iw_server_single, SIGINT);

  memset(&info, 0, sizeof info);
  info.port = 8848;
  info.protocols = protocols;
  
  info.options = LWS_SERVER_OPTION_LIBUV; 
  
  void* foreign_loops[1];
  foreign_loops[0] = loop;
  info.foreign_loops = foreign_loops;
  info.iface = "0.0.0.0";

  context = lws_create_context(&info);
  if (!context) {
    fprintf(stderr, "libwebsockets init failed\n");
    return -1;
  }

  printf("WebSocket Server started on ws://localhost:%d\n", info.port);
  printf("Press CTRL+C to stop.\n");

  uv_run(loop, UV_RUN_DEFAULT);

  lws_context_destroy(context);
  uv_loop_close(loop);

  return 0;
}
