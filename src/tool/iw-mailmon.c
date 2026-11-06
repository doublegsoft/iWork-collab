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
#include <stdint.h>
#include <pthread.h>
#include <dirent.h>
#include <signal.h>
#include <sys/stat.h>
#include <limits.h> /* 提供 PATH_MAX */

/* ==================== 跨平台兼容适配层 ==================== */
#ifndef _WIN32
  #include <unistd.h>
#else
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>  // 包含 Sleep
  #include <direct.h>   // 包含 _mkdir
  #include <io.h>       // 包含 _access

  // 1. 适配 Windows 下只有一个参数的 mkdir
  #define mkdir(path, mode) _mkdir(path)

  // 2. 适配 Windows 下的 realpath（注意参数顺序与 realpath 略有不同）
  static inline char *compat_realpath(const char *path, char *resolved_path) {
      return _fullpath(resolved_path, path, resolved_path ? PATH_MAX : 0);
  }
  #define realpath(path, resolved) compat_realpath(path, resolved)

  // 3. 适配 sleep 与 usleep（Windows 下使用 Sleep，单位毫秒）
  #define sleep(sec) Sleep((DWORD)((sec) * 1000))
  #define usleep(usec) Sleep((DWORD)(((usec) + 999) / 1000))

  // 4. 适配 access 与 F_OK（Windows 下使用 _access）
  #ifndef F_OK
    #define F_OK 0
  #endif
  #ifndef access
    #define access(path, mode) _access(path, mode)
  #endif

  // 5. 适配 S_ISDIR 与 S_ISREG 宏（MSVC 环境可能缺失）
  #ifndef S_ISDIR
    #define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
  #endif
  #ifndef S_ISREG
    #define S_ISREG(mode) (((mode) & S_IFMT) == S_IFREG)
  #endif

  // 如果没有定义 PATH_MAX，Windows 下通常使用 MAX_PATH
  #ifndef PATH_MAX
    #define PATH_MAX 4096
  #endif
#endif

#include "goolib-error.h"
#include "goolib-mail.h"

#define MD5_HEX_LEN 33

/* ==================== RFC 1321 MD5 纯 C 实现 ==================== */

typedef struct {
  uint32_t state[4];
  uint32_t count[2];
  uint8_t buffer[64];
} MD5Context;

#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))
#define ROTL(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

#define FF(a, b, c, d, x, s, ac) { \
  (a) += F((b), (c), (d)) + (x) + (uint32_t)(ac); \
  (a) = ROTL((a), (s)); \
  (a) += (b); \
}
#define GG(a, b, c, d, x, s, ac) { \
  (a) += G((b), (c), (d)) + (x) + (uint32_t)(ac); \
  (a) = ROTL((a), (s)); \
  (a) += (b); \
}
#define HH(a, b, c, d, x, s, ac) { \
  (a) += H((b), (c), (d)) + (x) + (uint32_t)(ac); \
  (a) = ROTL((a), (s)); \
  (a) += (b); \
}
#define II(a, b, c, d, x, s, ac) { \
  (a) += I((b), (c), (d)) + (x) + (uint32_t)(ac); \
  (a) = ROTL((a), (s)); \
  (a) += (b); \
}

static void md5_transform(uint32_t state[4], const uint8_t block[64]) {
  uint32_t a = state[0], b = state[1], c = state[2], d = state[3], x[16];
  for (int i = 0, j = 0; i < 16; i++, j += 4) {
    x[i] = ((uint32_t)block[j]) | (((uint32_t)block[j + 1]) << 8) |
           (((uint32_t)block[j + 2]) << 16) | (((uint32_t)block[j + 3]) << 24);
  }

  /* Round 1 */
  FF(a, b, c, d, x[0], 7, 0xd76aa478);
  FF(d, a, b, c, x[1], 12, 0xe8c7b756);
  FF(c, d, a, b, x[2], 17, 0x242070db);
  FF(b, c, d, a, x[3], 22, 0xc1bdceee);
  FF(a, b, c, d, x[4], 7, 0xf57c0faf);
  FF(d, a, b, c, x[5], 12, 0x4787c62a);
  FF(c, d, a, b, x[6], 17, 0xa8304613);
  FF(b, c, d, a, x[7], 22, 0xfd469501);
  FF(a, b, c, d, x[8], 7, 0x698098d8);
  FF(d, a, b, c, x[9], 12, 0x8b44f7af);
  FF(c, d, a, b, x[10], 17, 0xffff5bb1);
  FF(b, c, d, a, x[11], 22, 0x895cd7be);
  FF(a, b, c, d, x[12], 7, 0x6b901122);
  FF(d, a, b, c, x[13], 12, 0xfd987193);
  FF(c, d, a, b, x[14], 17, 0xa679438e);
  FF(b, c, d, a, x[15], 22, 0x49b40821);

  /* Round 2 */
  GG(a, b, c, d, x[1], 5, 0xf61e2562);
  GG(d, a, b, c, x[6], 9, 0xc040b340);
  GG(c, d, a, b, x[11], 14, 0x265e5a51);
  GG(b, c, d, a, x[0], 20, 0xe9b6c7aa);
  GG(a, b, c, d, x[5], 5, 0xd62f105d);
  GG(d, a, b, c, x[10], 9, 0x02441453);
  GG(c, d, a, b, x[15], 14, 0xd8a1e681);
  GG(b, c, d, a, x[4], 20, 0xe7d3fbc8);
  GG(a, b, c, d, x[9], 5, 0x21e1cde6);
  GG(d, a, b, c, x[14], 9, 0xc33707d6);
  GG(c, d, a, b, x[3], 14, 0xf4d50d87);
  GG(b, c, d, a, x[8], 20, 0x455a14ed);
  GG(a, b, c, d, x[13], 5, 0xa9e3e905);
  GG(d, a, b, c, x[2], 9, 0xfcefa3f8);
  GG(c, d, a, b, x[7], 14, 0x676f02d9);
  GG(b, c, d, a, x[12], 20, 0x8d2a4c8a);

  /* Round 3 */
  HH(a, b, c, d, x[5], 4, 0xfffa3942);
  HH(d, a, b, c, x[8], 11, 0x8771f681);
  HH(c, d, a, b, x[11], 16, 0x6d9d6122);
  HH(b, c, d, a, x[14], 23, 0xfde5380c);
  HH(a, b, c, d, x[1], 4, 0xa4beea44);
  HH(d, a, b, c, x[4], 11, 0x4bdecfa9);
  HH(c, d, a, b, x[7], 16, 0xf6bb4b60);
  HH(b, c, d, a, x[10], 23, 0xbebfbc70);
  HH(a, b, c, d, x[13], 4, 0x289b7ec6);
  HH(d, a, b, c, x[0], 11, 0xeaa127fa);
  HH(c, d, a, b, x[3], 16, 0xd4ef3085);
  HH(b, c, d, a, x[6], 23, 0x04881d05);
  HH(a, b, c, d, x[9], 4, 0xd9d4d039);
  HH(d, a, b, c, x[12], 11, 0xe6db99e5);
  HH(c, d, a, b, x[15], 16, 0x1fa27cf8);
  HH(b, c, d, a, x[2], 23, 0xc4ac5665);

  /* Round 4 */
  II(a, b, c, d, x[0], 6, 0xf4292244);
  II(d, a, b, c, x[7], 10, 0x432aff97);
  II(c, d, a, b, x[14], 15, 0xab9423a7);
  II(b, c, d, a, x[5], 21, 0xfc93a039);
  II(a, b, c, d, x[12], 6, 0x655b59c3);
  II(d, a, b, c, x[3], 10, 0x8f0ccc92);
  II(c, d, a, b, x[10], 15, 0xffeff47d);
  II(b, c, d, a, x[1], 21, 0x85845dd1);
  II(a, b, c, d, x[8], 6, 0x6fa87e4f);
  II(d, a, b, c, x[15], 10, 0xfe2ce6e0);
  II(c, d, a, b, x[6], 15, 0xa3014314);
  II(b, c, d, a, x[13], 21, 0x4e0811a1);
  II(a, b, c, d, x[4], 6, 0xf7537e82);
  II(d, a, b, c, x[11], 10, 0xbd3af235);
  II(c, d, a, b, x[2], 15, 0x2ad7d2bb);
  II(b, c, d, a, x[9], 21, 0xeb86d391);

  state[0] += a;
  state[1] += b;
  state[2] += c;
  state[3] += d;
}

static void md5_init(MD5Context* ctx) {
  ctx->count[0] = ctx->count[1] = 0;
  ctx->state[0] = 0x67452301;
  ctx->state[1] = 0xefcdab89;
  ctx->state[2] = 0x98badcfe;
  ctx->state[3] = 0x10325476;
}

static void md5_update(MD5Context* ctx, const uint8_t* input, size_t input_len) {
  uint32_t i = 0, index = 0, part_len = 0;
  index = (uint32_t)((ctx->count[0] >> 3) & 0x3F);
  if ((ctx->count[0] += ((uint32_t)input_len << 3)) < ((uint32_t)input_len << 3)) {
    ctx->count[1]++;
  }
  ctx->count[1] += ((uint32_t)input_len >> 29);
  part_len = 64 - index;

  if (input_len >= part_len) {
    memcpy(&ctx->buffer[index], input, part_len);
    md5_transform(ctx->state, ctx->buffer);
    for (i = part_len; i + 63 < input_len; i += 64) {
      md5_transform(ctx->state, &input[i]);
    }
    index = 0;
  }
  memcpy(&ctx->buffer[index], &input[i], input_len - i);
}

static void md5_final(MD5Context* ctx, uint8_t digest[16]) {
  static const uint8_t PADDING[64] = { 0x80 };
  uint8_t bits[8];
  for (int i = 0, j = 0; i < 2; i++, j += 4) {
    bits[j] = (uint8_t)(ctx->count[i] & 0xFF);
    bits[j + 1] = (uint8_t)((ctx->count[i] >> 8) & 0xFF);
    bits[j + 2] = (uint8_t)((ctx->count[i] >> 16) & 0xFF);
    bits[j + 3] = (uint8_t)((ctx->count[i] >> 24) & 0xFF);
  }

  uint32_t index = (uint32_t)((ctx->count[0] >> 3) & 0x3F);
  uint32_t pad_len = (index < 56) ? (56 - index) : (120 - index);
  md5_update(ctx, PADDING, pad_len);
  md5_update(ctx, bits, 8);

  for (int i = 0, j = 0; i < 4; i++, j += 4) {
    digest[j] = (uint8_t)(ctx->state[i] & 0xFF);
    digest[j + 1] = (uint8_t)((ctx->state[i] >> 8) & 0xFF);
    digest[j + 2] = (uint8_t)((ctx->state[i] >> 16) & 0xFF);
    digest[j + 3] = (uint8_t)((ctx->state[i] >> 24) & 0xFF);
  }
}

/* ==================== 文件追踪与业务处理 ==================== */

typedef struct FileNode {
  char filepath[PATH_MAX];
  struct FileNode* next;
} FileNode;

typedef struct {
  char filepath[PATH_MAX];
} TaskArgs;

static volatile sig_atomic_t g_running = 1;
static FileNode* g_tracked_list = NULL;
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

static void sig_handler(int sig) {
  (void)sig;
  g_running = 0;
}

static int is_tracked(const char* filepath) {
  pthread_mutex_lock(&g_mutex);
  FileNode* curr = g_tracked_list;
  while (curr) {
    if (strcmp(curr->filepath, filepath) == 0) {
      pthread_mutex_unlock(&g_mutex);
      return 1;
    }
    curr = curr->next;
  }
  pthread_mutex_unlock(&g_mutex);
  return 0;
}

static void add_tracked(const char* filepath) {
  pthread_mutex_lock(&g_mutex);
  FileNode* node = (FileNode*)malloc(sizeof(FileNode));
  if (node) {
    strncpy(node->filepath, filepath, sizeof(node->filepath) - 1);
    node->filepath[sizeof(node->filepath) - 1] = '\0';
    node->next = g_tracked_list;
    g_tracked_list = node;
  }
  pthread_mutex_unlock(&g_mutex);
}

/* 如果文件已在磁盘被删除，清理追踪记录，方便未来同名文件重新触发 */
static void cleanup_deleted_tracked(void) {
  pthread_mutex_lock(&g_mutex);
  FileNode** curr = &g_tracked_list;
  while (*curr) {
    if (access((*curr)->filepath, F_OK) != 0) {
      FileNode* to_free = *curr;
      *curr = (*curr)->next;
      free(to_free);
    } else {
      curr = &(*curr)->next;
    }
  }
  pthread_mutex_unlock(&g_mutex);
}

static int calculate_md5(const char* filepath, char* output_hash) {
  FILE* fp = fopen(filepath, "rb");
  if (!fp) {
    return -1;
  }

  MD5Context ctx;
  md5_init(&ctx);

  uint8_t buffer[8192];
  size_t bytes_read = 0;
  while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
    md5_update(&ctx, buffer, bytes_read);
  }

  uint8_t digest[16];
  md5_final(&ctx, digest);
  fclose(fp);

  for (int i = 0; i < 16; i++) {
    sprintf(output_hash + (i * 2), "%02x", digest[i]);
  }
  output_hash[32] = '\0';
  return 0;
}

static void process_file(const char* filepath, const char* final_md5) {
  printf("\n[成功处理] 文件写入完成并就绪!\n");
  printf(" -> 文件路径: %s\n", filepath);
  printf(" -> 最终 MD5: %s\n\n", final_md5);

  char* error = NULL;
  goo_mail_send("smtp.qq.com", "465", 
                "598807227@qq.com", "dxufwgbzlwghbehc", 
                "598807227@qq.com", "598807227@qq.com", 
                "附件传输", 
                "NO", filepath, 
                &error);
  
  if (error != NULL) {
    printf("error: %s\n", error);
    free(error);
  }

  if (remove(filepath) == 0) {
    printf("File '%s' deleted successfully.\n", filepath);
  } 
}

static void* verify_and_process(void* arg) {
  TaskArgs* task = (TaskArgs*)arg;
  char current_md5[MD5_HEX_LEN];
  char last_md5[MD5_HEX_LEN] = "";
  int consecutive_matches = 0;

  printf("[开始监控文件] %s\n", task->filepath);

  while (g_running && consecutive_matches < 3) {
    if (access(task->filepath, F_OK) != 0) {
      printf("[警告] 文件丢失或已被移除: %s\n", task->filepath);
      break;
    }

    if (calculate_md5(task->filepath, current_md5) != 0) {
      consecutive_matches = 0;
      last_md5[0] = '\0';
      printf("[读取失败] 文件正在被独占写入，重置计数: %s\n", task->filepath);
    } else if (strcmp(current_md5, last_md5) == 0) {
      consecutive_matches++;
      printf("[匹配成功 %d/3] MD5: %s - %s\n", consecutive_matches, current_md5, task->filepath);
    } else {
      consecutive_matches = 1;
      strncpy(last_md5, current_md5, MD5_HEX_LEN);
      printf("[内容变动/重新计数 1/3] 最新 MD5: %s - %s\n", current_md5, task->filepath);
    }

    if (consecutive_matches == 3) {
      break;
    }

    sleep(1);
  }

  if (consecutive_matches == 3) {
    process_file(task->filepath, last_md5);
  }

  free(task);
  return NULL;
}

/* 启动监控线程 */
static void spawn_monitor_task(const char* fullpath) {
  if (is_tracked(fullpath)) {
    return;
  }
  add_tracked(fullpath);

  TaskArgs* task = (TaskArgs*)malloc(sizeof(TaskArgs));
  if (task) {
    strncpy(task->filepath, fullpath, sizeof(task->filepath) - 1);
    task->filepath[sizeof(task->filepath) - 1] = '\0';

    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (pthread_create(&tid, &attr, verify_and_process, task) != 0) {
      perror("pthread_create");
      free(task);
    }
    pthread_attr_destroy(&attr);
  }
}

/* 递归扫描目录（子目录同样处理） */
static void scan_directory_recursive(const char* base_dir) {
  DIR* dir = opendir(base_dir);
  if (!dir) return;

  struct dirent* entry;
  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    char fullpath[PATH_MAX];
    snprintf(fullpath, sizeof(fullpath), "%s/%s", base_dir, entry->d_name);

    struct stat file_st;
    if (stat(fullpath, &file_st) == 0) {
      if (S_ISREG(file_st.st_mode)) {
        spawn_monitor_task(fullpath);
      } else if (S_ISDIR(file_st.st_mode)) {
        /* 子目录同样做递归扫描处理 */
        scan_directory_recursive(fullpath);
      }
    }
  }
  closedir(dir);
}

int main(int argc, char* argv[]) {
  char watch_target[PATH_MAX] = "./incoming_files";
  if (argc > 1) {
    strncpy(watch_target, argv[1], sizeof(watch_target) - 1);
    watch_target[sizeof(watch_target) - 1] = '\0';
  }

  signal(SIGINT, sig_handler);
  signal(SIGTERM, sig_handler);

  /* 1. 检查目标是否存在，不存在则尝试作为目录自动创建 */
  struct stat st;
  if (stat(watch_target, &st) == -1) {
    if (mkdir(watch_target, 0755) != 0) {
      perror("创建目标目录失败");
      return 1;
    }
    stat(watch_target, &st);
  }

  /* 2. 转换为完整绝对路径 */
  char resolved_path[PATH_MAX];
  if (realpath(watch_target, resolved_path) != NULL) {
    strncpy(watch_target, resolved_path, sizeof(watch_target) - 1);
    watch_target[sizeof(watch_target) - 1] = '\0';
  } else {
    perror("警告: 无法获取完整绝对路径 (realpath)");
  }

  /* 3. 如果传入的是单个已有文件，同样做防写入校验、发送邮件并删除 */
  if (S_ISREG(st.st_mode)) {
    printf("检测到目标是单个文件，执行同样的处理流程: %s\n", watch_target);
    TaskArgs* single_task = (TaskArgs*)malloc(sizeof(TaskArgs));
    if (single_task) {
      strncpy(single_task->filepath, watch_target, sizeof(single_task->filepath) - 1);
      single_task->filepath[sizeof(single_task->filepath) - 1] = '\0';
      verify_and_process(single_task);
    }
    return 0;
  }

  /* 4. 如果是目录，开启持续循环监听（支持子目录递归） */
  printf("正在跨平台监听目录: %s ... (按 Ctrl+C 退出)\n", watch_target);

  while (g_running) {
    scan_directory_recursive(watch_target);
    cleanup_deleted_tracked();
    usleep(500000); /* 每 500ms 扫描一次文件夹 */
  }

  printf("\n退出监听，释放内存...\n");
  pthread_mutex_lock(&g_mutex);
  FileNode* curr = g_tracked_list;
  while (curr) {
    FileNode* tmp = curr;
    curr = curr->next;
    free(tmp);
  }
  g_tracked_list = NULL;
  pthread_mutex_unlock(&g_mutex);

  return 0;
}