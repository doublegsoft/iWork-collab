#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#define iw_mkdir(path) _mkdir(path)
#else
#include <sys/stat.h>
#include <sys/types.h>
#define iw_mkdir(path) mkdir(path, 0755)
#endif

#define MAX_PATH_LEN 1024
#define MAX_LINE_LEN 4096

typedef enum {
  STATE_SEEK_PATH = 0,
  STATE_SEEK_FENCE,
  STATE_READ_CONTENT
} parse_state_t;

static void trim_line(char* str) {
  if (!str) return;
  size_t len = strlen(str);
  while (len > 0 && (str[len - 1] == '\r' || str[len - 1] == '\n' || isspace((unsigned char)str[len - 1]))) {
    str[--len] = '\0';
  }
}

static void ensure_parent_dirs(const char* filepath) {
  char temp[MAX_PATH_LEN];
  strncpy(temp, filepath, sizeof(temp) - 1);
  temp[sizeof(temp) - 1] = '\0';

  for (char* p = temp; *p; p++) {
    if (*p == '/' || *p == '\\') {
      char old_ch = *p;
      *p = '\0';
      if (strlen(temp) > 0) {
        iw_mkdir(temp);
      }
      *p = old_ch;
    }
  }
}

static void build_target_path(char* out, size_t max_len, const char* base_dir, const char* rel_path) {
  if (!base_dir || strlen(base_dir) == 0 || strcmp(base_dir, ".") == 0) {
    strncpy(out, rel_path, max_len - 1);
  } else {
    size_t dlen = strlen(base_dir);
    if (base_dir[dlen - 1] == '/' || base_dir[dlen - 1] == '\\') {
      snprintf(out, max_len, "%s%s", base_dir, rel_path);
    } else {
      snprintf(out, max_len, "%s/%s", base_dir, rel_path);
    }
  }
  out[max_len - 1] = '\0';
}

static bool is_code_fence(const char* line) {
  while (*line && isspace((unsigned char)*line)) {
    line++;
  }
  return (strncmp(line, "```", 3) == 0);
}

static void print_usage(const char* prog_name) {
  printf("Usage: %s <input_file> [output_directory]\n", prog_name);
  printf("  <input_file>       Input text file containing paste/markdown blocks (use '-' for stdin)\n");
  printf("  [output_directory] Target root directory to extract files (default: current directory '.')\n");
}

int main(int argc, char* const* argv) {
  if (argc < 2) {
    print_usage(argv[0]);
    return EXIT_FAILURE;
  }

  const char* in_filename = argv[1];
  const char* out_directory = (argc >= 3) ? argv[2] : ".";

  FILE* in = NULL;
  if (strcmp(in_filename, "-") == 0) {
    in = stdin;
  } else {
    in = fopen(in_filename, "r");
    if (!in) {
      fprintf(stderr, "Error: Unable to open input file: %s\n", in_filename);
      return EXIT_FAILURE;
    }
  }

  /* 如果指定的输出目录不是当前目录，先确保该根目录存在 */
  if (strcmp(out_directory, ".") != 0) {
    char root_dir_probe[MAX_PATH_LEN];
    snprintf(root_dir_probe, sizeof(root_dir_probe), "%s/", out_directory);
    ensure_parent_dirs(root_dir_probe);
  }

  char line[MAX_LINE_LEN];
  char relative_path[MAX_PATH_LEN] = {0};
  char final_path[MAX_PATH_LEN] = {0};
  FILE* out_file = NULL;
  parse_state_t state = STATE_SEEK_PATH;
  int generated_count = 0;

  while (fgets(line, sizeof(line), in)) {
    char trimmed[MAX_LINE_LEN];
    strncpy(trimmed, line, sizeof(trimmed) - 1);
    trimmed[sizeof(trimmed) - 1] = '\0';
    trim_line(trimmed);

    switch (state) {
      case STATE_SEEK_PATH: {
        if (strlen(trimmed) == 0) continue;
        if (strncmp(trimmed, "---", 3) == 0) continue;
        if (is_code_fence(trimmed)) continue;

        /* 获取相对路径并计算拼接后的最终绝对/相对落盘路径 */
        strncpy(relative_path, trimmed, sizeof(relative_path) - 1);
        relative_path[sizeof(relative_path) - 1] = '\0';
        build_target_path(final_path, sizeof(final_path), out_directory, relative_path);

        state = STATE_SEEK_FENCE;
        break;
      }

      case STATE_SEEK_FENCE: {
        if (strlen(trimmed) == 0) continue;

        if (is_code_fence(trimmed)) {
          ensure_parent_dirs(final_path);
          out_file = fopen(final_path, "wb");
          if (!out_file) {
            fprintf(stderr, "Error: Failed to create target file: %s\n", final_path);
            state = STATE_SEEK_PATH;
            relative_path[0] = '\0';
            final_path[0] = '\0';
          } else {
            state = STATE_READ_CONTENT;
          }
        } else {
          /* 若下一行仍不是代码块起始，说明前一行可能是注释，更新为当前行 */
          strncpy(relative_path, trimmed, sizeof(relative_path) - 1);
          relative_path[sizeof(relative_path) - 1] = '\0';
          build_target_path(final_path, sizeof(final_path), out_directory, relative_path);
        }
        break;
      }

      case STATE_READ_CONTENT: {
        if (is_code_fence(trimmed)) {
          if (out_file) {
            fclose(out_file);
            out_file = NULL;
            printf("[Extracted] %s -> %s\n", relative_path, final_path);
            generated_count++;
          }
          relative_path[0] = '\0';
          final_path[0] = '\0';
          state = STATE_SEEK_PATH;
        } else {
          if (out_file) {
            fputs(line, out_file);
          }
        }
        break;
      }
    }
  }

  if (out_file) {
    fclose(out_file);
    printf("[Extracted] %s -> %s\n", relative_path, final_path);
    generated_count++;
  }

  if (in != stdin) {
    fclose(in);
  }

  printf("\nExtraction finished. Total files: %d, Target directory: '%s'\n", generated_count, out_directory);
  return EXIT_SUCCESS;
}