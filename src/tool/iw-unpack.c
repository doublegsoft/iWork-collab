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

/*!
** 去除行尾的换行符（\r、\n）及尾随空白字符。
**
** @param str 待修剪的目标字符串指针
*/
static void 
iw_trim_line(char* str) {
  if (!str) return;
  size_t len = strlen(str);
  while (len > 0 && (str[len - 1] == '\r' || str[len - 1] == '\n' || isspace((unsigned char)str[len - 1]))) {
    str[--len] = '\0';
  }
}

/*!
** 递归检查并创建指定文件路径所依赖的所有上级父目录。
**
** @param filepath 包含目录与文件名的路径字符串
*/
static void iw_ensure_dirs(const char* filepath) {
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

/*!
** 将基础输出目录与文件相对路径拼接组合为目标落盘路径。
**
** @param out      存储拼接结果的目标缓冲区
** @param max_len  输出缓冲区最大长度
** @param base_dir 根目录路径（若为空或 "." 则不额外追加前缀）
** @param rel_path 待拼接的相对文件路径
*/
static void iw_build_target_path(char* out, size_t max_len, const char* base_dir, const char* rel_path) {
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

/*!
** 判断文本行是否为 Markdown 格式的代码块栅栏标记（以 ``` 开头）。
**
** @param line 待检测的文本行指针
** @return 若为代码块起止标记则返回 true，否则返回 false
*/
static bool iw_is_code_fence(const char* line) {
  while (*line && isspace((unsigned char)*line)) {
    line++;
  }
  return (strncmp(line, "```", 3) == 0);
}

/*!
** 打印程序的命令行帮助与使用说明信息。
**
** @param prog_name 当前执行的程序名
*/
static void iw_print_usage(const char* prog_name) {
  printf("Usage: %s <input_file> [output_directory]\n", prog_name);
  printf("  <input_file>       Input text file containing paste/markdown blocks (use '-' for stdin)\n");
  printf("  [output_directory] Target root directory to extract files (default: current directory '.')\n");
}

/*!
** 程序主入口：解析输入流/文件中的 Markdown 文本块，提取代码段并还原到指定目录结构中。
**
** @param argc 命令行参数个数
** @param argv 命令行参数字符串数组
** @return 成功返回 EXIT_SUCCESS，失败返回 EXIT_FAILURE
*/
int main(int argc, char* const* argv) {
  if (argc < 2) {
    iw_print_usage(argv[0]);
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
    iw_ensure_dirs(root_dir_probe);
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
    iw_trim_line(trimmed);

    switch (state) {
      case STATE_SEEK_PATH: {
        if (strlen(trimmed) == 0) continue;
        if (strncmp(trimmed, "---", 3) == 0) continue;
        if (iw_is_code_fence(trimmed)) continue;

        /* 获取相对路径并计算拼接后的最终绝对/相对落盘路径 */
        strncpy(relative_path, trimmed, sizeof(relative_path) - 1);
        relative_path[sizeof(relative_path) - 1] = '\0';
        iw_build_target_path(final_path, sizeof(final_path), out_directory, relative_path);

        state = STATE_SEEK_FENCE;
        break;
      }

      case STATE_SEEK_FENCE: {
        if (strlen(trimmed) == 0) continue;

        if (iw_is_code_fence(trimmed)) {
          iw_ensure_dirs(final_path);
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
          iw_build_target_path(final_path, sizeof(final_path), out_directory, relative_path);
        }
        break;
      }

      case STATE_READ_CONTENT: {
        if (iw_is_code_fence(trimmed)) {
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