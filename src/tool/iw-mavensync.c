#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>

// 递归创建目录 (mkdir -p)
static void mkdir_p(const char* dir_path) {
  char tmp[PATH_MAX];
  char* p = NULL;
  size_t len;

  snprintf(tmp, sizeof(tmp), "%s", dir_path);
  len = strlen(tmp);
  if (tmp[len - 1] == '/') {
    tmp[len - 1] = 0;
  }
  for (p = tmp + 1; *p; p++) {
    if (*p == '/') {
      *p = 0;
      mkdir(tmp, 0755);
      *p = '/';
    }
  }
  mkdir(tmp, 0755);
}

// 拷贝文件
static int copy_file(const char* src, const char* dst) {
  FILE* in = fopen(src, "rb");
  if (!in) {
    return -1;
  }

  // 确保目标所在目录存在
  char dst_dir[PATH_MAX];
  snprintf(dst_dir, sizeof(dst_dir), "%s", dst);
  char* last_slash = strrchr(dst_dir, '/');
  if (last_slash) {
    *last_slash = '\0';
    mkdir_p(dst_dir);
  }

  FILE* out = fopen(dst, "wb");
  if (!out) {
    fclose(in);
    return -1;
  }

  char buf[8192];
  size_t n;
  while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
    fwrite(buf, 1, n, out);
  }

  fclose(in);
  fclose(out);
  return 0;
}

// 解析时间参数，支持时间戳或 "YYYY-MM-DD HH:MM:SS"
static time_t parse_time_arg(const char* str) {
  struct tm tm_buf;
  memset(&tm_buf, 0, sizeof(struct tm));

  // 尝试按秒级时间戳解析
  char* endptr;
  long val = strtol(str, &endptr, 10);
  if (*endptr == '\0' && val > 0) {
    return (time_t)val;
  }

  // 尝试按日期时间格式解析
  if (sscanf(str, "%d-%d-%d %d:%d:%d",
             &tm_buf.tm_year, &tm_buf.tm_mon, &tm_buf.tm_mday,
             &tm_buf.tm_hour, &tm_buf.tm_min, &tm_buf.tm_sec) >= 3) {
    tm_buf.tm_year -= 1900;
    tm_buf.tm_mon -= 1;
    tm_buf.tm_isdst = -1;
    return mktime(&tm_buf);
  }

  return (time_t)-1;
}

// 提取类文件并拷贝
static void handle_matched_java(const char* java_path, const char* proj_root, const char* dest_dir) {
  // 查找 Maven 约定源码目录
  const char* main_tag = "/src/main/java/";
  const char* test_tag = "/src/test/java/";
  const char* hit = strstr(java_path, main_tag);
  const char* target_base = "target/classes/";

  if (hit) {
    hit += strlen(main_tag);
  } else {
    hit = strstr(java_path, test_tag);
    if (hit) {
      hit += strlen(test_tag);
      target_base = "target/test-classes/";
    } else {
      return; // 非标准 maven 路径，跳过
    }
  }

  // 分离包路径与类名
  char rel_pkg_dir[PATH_MAX] = {0};
  char class_prefix[256] = {0};
  const char* last_slash = strrchr(hit, '/');

  if (last_slash) {
    strncpy(rel_pkg_dir, hit, last_slash - hit);
    const char* filename = last_slash + 1;
    char* dot = strrchr(filename, '.');
    if (dot) {
      strncpy(class_prefix, filename, dot - filename);
    }
  } else {
    char* dot = strrchr(hit, '.');
    if (dot) {
      strncpy(class_prefix, hit, dot - hit);
    }
  }

  // 构造 target 下 class 文件所在的目录
  char target_dir[PATH_MAX];
  if (strlen(rel_pkg_dir) > 0) {
    snprintf(target_dir, sizeof(target_dir), "%s/%s%s", proj_root, target_base, rel_pkg_dir);
  } else {
    snprintf(target_dir, sizeof(target_dir), "%s/%s", proj_root, target_base);
  }

  DIR* dir = opendir(target_dir);
  if (!dir) {
    return;
  }

  // 扫描包括 ClassName.class 及 ClassName$Inner.class
  struct dirent* entry;
  size_t prefix_len = strlen(class_prefix);

  while ((entry = readdir(dir)) != NULL) {
    // 忽略 "." 和 ".."
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    char src_class_path[PATH_MAX];
    snprintf(src_class_path, sizeof(src_class_path), "%s/%s", target_dir, entry->d_name);

    // 使用标准 stat 代替非标准的 d_type，保证绝对可移植
    struct stat file_st;
    if (stat(src_class_path, &file_st) != 0 || !S_ISREG(file_st.st_mode)) {
      continue;
    }

    if (strncmp(entry->d_name, class_prefix, prefix_len) == 0) {
      char next_char = entry->d_name[prefix_len];
      if (next_char == '.' || next_char == '$') {
        if (strstr(entry->d_name, ".class")) {
          char dst_class_path[PATH_MAX];

          if (strlen(rel_pkg_dir) > 0) {
            snprintf(dst_class_path, sizeof(dst_class_path), "%s/%s/%s", dest_dir, rel_pkg_dir, entry->d_name);
          } else {
            snprintf(dst_class_path, sizeof(dst_class_path), "%s/%s", dest_dir, entry->d_name);
          }

          if (copy_file(src_class_path, dst_class_path) == 0) {
            char resolved_dst[PATH_MAX];
            if (realpath(dst_class_path, resolved_dst)) {
              printf("[FOUND & COPIED] %s\n", resolved_dst);
            } else {
              printf("[FOUND & COPIED] %s\n", dst_class_path);
            }
          }
        }
      }
    }
  }
  closedir(dir);
}

// 递归遍历项目源码
static void scan_directory(const char* current_path, time_t threshold, const char* proj_root, const char* dest_dir) {
  DIR* dir = opendir(current_path);
  if (!dir) {
    return;
  }

  struct dirent* entry;
  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    char full_path[PATH_MAX];
    snprintf(full_path, sizeof(full_path), "%s/%s", current_path, entry->d_name);

    // 过滤构建输出目录和 VCS 目录
    if (strcmp(entry->d_name, "target") == 0 || strcmp(entry->d_name, ".git") == 0) {
      continue;
    }

    struct stat st;
    if (stat(full_path, &st) == -1) {
      continue;
    }

    if (S_ISDIR(st.st_mode)) {
      scan_directory(full_path, threshold, proj_root, dest_dir);
    } else if (S_ISREG(st.st_mode)) {
      char* ext = strrchr(entry->d_name, '.');
      if (ext && strcmp(ext, ".java") == 0) {
        if (st.st_mtime > threshold) {
          handle_matched_java(full_path, proj_root, dest_dir);
        }
      }
    }
  }
  closedir(dir);
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <time_param> [target_output_dir] [project_root]\n", argv[0]);
    fprintf(stderr, "Example:\n");
    fprintf(stderr, "  %s \"2023-10-01 12:00:00\"\n", argv[0]);
    fprintf(stderr, "  %s 1696132800 ./output_classes /path/to/maven_project\n", argv[0]);
    return 1;
  }

  time_t threshold = parse_time_arg(argv[1]);
  if (threshold == (time_t)-1) {
    fprintf(stderr, "Error: Invalid time format. Use timestamp or 'YYYY-MM-DD HH:MM:SS'.\n");
    return 1;
  }

  const char* dest_dir = (argc >= 3) ? argv[2] : "./output_classes";
  const char* proj_root = (argc >= 4) ? argv[3] : ".";

  char abs_proj_root[PATH_MAX];
  char abs_dest_dir[PATH_MAX];

  mkdir_p(dest_dir);

  if (!realpath(proj_root, abs_proj_root)) {
    perror("Project root path error");
    return 1;
  }
  if (!realpath(dest_dir, abs_dest_dir)) {
    perror("Destination directory error");
    return 1;
  }

  printf("Scanning Maven Project : %s\n", abs_proj_root);
  printf("Modified After         : %s", ctime(&threshold));
  printf("Target Output Directory: %s\n", abs_dest_dir);
  printf("------------------------------------------------------------------\n");

  scan_directory(abs_proj_root, threshold, abs_proj_root, abs_dest_dir);

  printf("------------------------------------------------------------------\n");
  printf("Done.\n");

  return 0;
}
