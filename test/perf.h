/*
**    ▄▄▄▄  ▄▄▄  ▄▄▄▄                   
** ▀▀ ▀███  ███  ███▀            ▄▄     
** ██  ███  ███  ███ ▄███▄ ████▄ ██ ▄█▀ 
** ██  ███▄▄███▄▄███ ██ ██ ██ ▀▀ ████   
** ██▄  ▀████▀████▀  ▀███▀ ██    ██ ▀█▄ 
*/
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/* 版本 */
#define WS_PERF_VERSION "1.0.0"

/* 默认配置 */
#define DEFAULT_DURATION_SEC    30
#define DEFAULT_CONCURRENCY     100
#define DEFAULT_MESSAGE_SIZE    1024
#define DEFAULT_MESSAGE_RATE    0

/* 最大限制 */
#define MAX_CONCURRENCY         100000
#define MAX_MESSAGE_SIZE        (1024 * 1024 * 10)
#define MAX_URL_LEN             2048

/* 测试模式 */
typedef enum {
  MODE_ECHO,
  MODE_BROADCAST,
  MODE_PING_PONG,
  MODE_THROUGHPUT
} test_mode_t;

/* 协议类型 */
typedef enum {
  PROTO_WS,
  PROTO_WSS,  /* 不支持，需要 OpenSSL */
  PROTO_TCP,
  PROTO_UDP
} protocol_t;

/* 平台检测 */
#ifdef __APPLE__
  #define PLATFORM_MACOS
  #include <sys/event.h>
  typedef int event_handle_t;
#elif defined(__linux__)
  #define PLATFORM_LINUX
  #include <sys/epoll.h>
  typedef int event_handle_t;
#else
  #error "Unsupported platform"
#endif

/* 统计指标 */
typedef struct {
  struct timespec start_time;
  struct timespec end_time;
  double duration_sec;
  
  uint64_t connections_attempted;
  uint64_t connections_success;
  uint64_t connections_failed;
  uint64_t connections_active;
  uint64_t connections_closed;
  double connect_time_ms_avg;
  double connect_time_ms_min;
  double connect_time_ms_max;
  
  uint64_t messages_sent;
  uint64_t messages_received;
  uint64_t messages_echoed;
  uint64_t messages_lost;
  uint64_t messages_errors;
  uint64_t bytes_sent;
  uint64_t bytes_received;
  
  double latency_us_avg;
  double latency_us_min;
  double latency_us_max;
  double latency_us_p50;
  double latency_us_p95;
  double latency_us_p99;
  double latency_us_stddev;
  
  double throughput_mbps;
  double msg_rate_send;
  double msg_rate_recv;
  
  uint64_t errors_timeout;
  uint64_t errors_read;
  uint64_t errors_write;
  uint64_t errors_protocol;
  
  double cpu_percent;
  uint64_t memory_mb;
  uint64_t open_files;
} perf_stats_t;

/* 直方图 */
typedef struct {
  uint32_t* buckets;
  int bucket_count;
  double bucket_size_us;
  uint64_t total_samples;
  double min_val;
  double max_val;
} latency_histogram_t;

/* 连接状态 */
typedef enum {
  CONN_IDLE,
  CONN_CONNECTING,
  CONN_HANDSHAKE,
  CONN_OPEN,
  CONN_SENDING,
  CONN_WAITING,
  CONN_RECEIVING,
  CONN_CLOSING,
  CONN_CLOSED,
  CONN_ERROR
} conn_state_t;

/* 连接上下文 */
typedef struct connection {
  int id;
  int fd;
  conn_state_t state;
  
  struct timespec ts_connect_start;
  struct timespec ts_connect_end;
  struct timespec ts_send;
  struct timespec ts_recv;
  
  uint8_t* send_buf;
  uint8_t* recv_buf;
  size_t send_len;
  size_t recv_len;
  size_t send_pos;
  size_t recv_pos;
  
  uint8_t ws_frame[14];
  size_t ws_frame_len;
  size_t ws_frame_pos;
  uint64_t ws_payload_len;
  uint8_t ws_opcode;
  bool ws_masked;
  
  uint64_t seq_num;
  uint64_t messages_sent;
  uint64_t messages_received;
  uint64_t bytes_sent;
  uint64_t bytes_received;
  
  double* latency_samples;
  size_t latency_sample_count;
  size_t latency_sample_capacity;
  
  perf_stats_t* stats;
  
  struct connection* next;
  struct connection* prev;
} connection_t;

/* 事件循环抽象 */
typedef struct {
  event_handle_t handle;
  void* events;
  int max_events;
  int event_size;
} event_loop_t;

/* 线程工作器 */
typedef struct worker {
  int id;
  pthread_t thread;
  
  event_loop_t evloop;
  
  connection_t* conns;
  connection_t* free_list;
  connection_t* active_list;
  int conn_count;
  int max_conns;
  
  perf_stats_t stats;
  latency_histogram_t histogram;
  
  volatile bool running;
  volatile bool should_stop;
  
  struct perf_test* test;
} worker_t;

/* 主测试上下文 */
typedef struct perf_test {
  char url[MAX_URL_LEN];
  test_mode_t mode;
  protocol_t proto;
  int duration_sec;
  int concurrency;
  int message_size;
  int message_rate;
  bool use_masking;
  bool verify_echo;
  bool binary_mode;
  
  char host[256];
  int port;
  char path[1024];
  
  bool use_tls;
  
  worker_t* workers;
  int worker_count;
  
  perf_stats_t total_stats;
  latency_histogram_t total_histogram;
  
  pthread_mutex_t stats_mutex;
  pthread_cond_t start_cond;
  volatile int ready_workers;
  volatile bool test_running;
  
  volatile bool interrupted;
  char* err_msg;
} perf_test_t;

/* API */
int perf_test_init(perf_test_t* test, int argc, char** argv);
int perf_test_run(perf_test_t* test);
void perf_test_report(perf_test_t* test);
void perf_test_cleanup(perf_test_t* test);

/* 工具函数 */
double timespec_diff_ms(struct timespec* start, struct timespec* end);
double timespec_diff_us(struct timespec* start, struct timespec* end);
void timespec_now(struct timespec* ts);
uint64_t get_time_ns(void);

/* 事件循环抽象 */
int evloop_init(event_loop_t* loop, int max_events);
void evloop_close(event_loop_t* loop);
int evloop_add(event_loop_t* loop, int fd, void* data, bool write);
int evloop_mod(event_loop_t* loop, int fd, void* data, bool write);
int evloop_del(event_loop_t* loop, int fd);
int evloop_wait(event_loop_t* loop, int timeout_ms);
