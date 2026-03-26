#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <math.h>

#include "perf.h"

#define WS_OP_CONTINUATION  0x0
#define WS_OP_TEXT          0x1
#define WS_OP_BINARY        0x2
#define WS_OP_CLOSE         0x8
#define WS_OP_PING          0x9
#define WS_OP_PONG          0xA

static volatile int g_interrupted = 0;

static void signal_handler(int sig) {
  g_interrupted = 1;
}

void timespec_now(struct timespec* ts) {
  clock_gettime(CLOCK_MONOTONIC, ts);
}

uint64_t get_time_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

double timespec_diff_ms(struct timespec* start, struct timespec* end) {
  return (end->tv_sec - start->tv_sec) * 1000.0 + 
         (end->tv_nsec - start->tv_nsec) / 1000000.0;
}

double timespec_diff_us(struct timespec* start, struct timespec* end) {
  return (end->tv_sec - start->tv_sec) * 1000000.0 + 
         (end->tv_nsec - start->tv_nsec) / 1000.0;
}

/* ============ 跨平台事件循环实现 ============ */

#ifdef PLATFORM_MACOS

int evloop_init(event_loop_t* loop, int max_events) {
  loop->handle = kqueue();
  if (loop->handle < 0) return -1;
  
  loop->max_events = max_events;
  loop->event_size = sizeof(struct kevent);
  loop->events = calloc(max_events, loop->event_size);
  
  return loop->events ? 0 : -1;
}

void evloop_close(event_loop_t* loop) {
  if (loop->handle >= 0) {
    close(loop->handle);
    loop->handle = -1;
  }
  free(loop->events);
  loop->events = NULL;
}

int evloop_add(event_loop_t* loop, int fd, void* data, bool write) {
  struct kevent ev[2];
  int n = 0;
  
  EV_SET(&ev[n++], fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, data);
  
  if (write) {
    EV_SET(&ev[n++], fd, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, data);
  } else {
    EV_SET(&ev[n++], fd, EVFILT_WRITE, EV_ADD | EV_DISABLE, 0, 0, data);
  }
  
  return kevent(loop->handle, ev, n, NULL, 0, NULL);
}

int evloop_mod(event_loop_t* loop, int fd, void* data, bool write) {
  struct kevent ev[2];
  int n = 0;
  
  EV_SET(&ev[n++], fd, EVFILT_READ, EV_ADD | (write ? EV_DISABLE : EV_ENABLE), 0, 0, data);
  EV_SET(&ev[n++], fd, EVFILT_WRITE, EV_ADD | (write ? EV_ENABLE : EV_DISABLE), 0, 0, data);
  
  return kevent(loop->handle, ev, n, NULL, 0, NULL);
}

int evloop_del(event_loop_t* loop, int fd) {
  struct kevent ev[2];
  EV_SET(&ev[0], fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
  EV_SET(&ev[1], fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
  kevent(loop->handle, ev, 2, NULL, 0, NULL);
  return 0;
}

int evloop_wait(event_loop_t* loop, int timeout_ms) {
  struct timespec ts;
  ts.tv_sec = timeout_ms / 1000;
  ts.tv_nsec = (timeout_ms % 1000) * 1000000;
  
  return kevent(loop->handle, NULL, 0, loop->events, loop->max_events, 
                timeout_ms < 0 ? NULL : &ts);
}

static inline void* evloop_get_data(event_loop_t* loop, int idx) {
  struct kevent* ev = (struct kevent*)loop->events + idx;
  return ev->udata;
}

static inline bool evloop_is_error(event_loop_t* loop, int idx) {
  struct kevent* ev = (struct kevent*)loop->events + idx;
  return (ev->flags & EV_ERROR) || (ev->flags & EV_EOF);
}

static inline bool evloop_is_readable(event_loop_t* loop, int idx) {
  struct kevent* ev = (struct kevent*)loop->events + idx;
  return ev->filter == EVFILT_READ;
}

static inline bool evloop_is_writable(event_loop_t* loop, int idx) {
  struct kevent* ev = (struct kevent*)loop->events + idx;
  return ev->filter == EVFILT_WRITE;
}

#else /* PLATFORM_LINUX */

int evloop_init(event_loop_t* loop, int max_events) {
  loop->handle = epoll_create1(EPOLL_CLOEXEC);
  if (loop->handle < 0) return -1;
  
  loop->max_events = max_events;
  loop->event_size = sizeof(struct epoll_event);
  loop->events = calloc(max_events, loop->event_size);
  
  return loop->events ? 0 : -1;
}

void evloop_close(event_loop_t* loop) {
  if (loop->handle >= 0) {
    close(loop->handle);
    loop->handle = -1;
  }
  free(loop->events);
  loop->events = NULL;
}

int evloop_add(event_loop_t* loop, int fd, void* data, bool write) {
  struct epoll_event ev = {
    .events = EPOLLIN | (write ? EPOLLOUT : 0) | EPOLLET,
    .data.ptr = data
  };
  return epoll_ctl(loop->handle, EPOLL_CTL_ADD, fd, &ev);
}

int evloop_mod(event_loop_t* loop, int fd, void* data, bool write) {
  struct epoll_event ev = {
    .events = (write ? EPOLLOUT : EPOLLIN) | EPOLLET,
    .data.ptr = data
  };
  return epoll_ctl(loop->handle, EPOLL_CTL_MOD, fd, &ev);
}

int evloop_del(event_loop_t* loop, int fd) {
  return epoll_ctl(loop->handle, EPOLL_CTL_DEL, fd, NULL);
}

int evloop_wait(event_loop_t* loop, int timeout_ms) {
  return epoll_wait(loop->handle, loop->events, loop->max_events, timeout_ms);
}

static inline void* evloop_get_data(event_loop_t* loop, int idx) {
  struct epoll_event* ev = (struct epoll_event*)loop->events + idx;
  return ev->data.ptr;
}

static inline bool evloop_is_error(event_loop_t* loop, int idx) {
  struct epoll_event* ev = (struct epoll_event*)loop->events + idx;
  return ev->events & (EPOLLERR | EPOLLHUP);
}

static inline bool evloop_is_readable(event_loop_t* loop, int idx) {
  struct epoll_event* ev = (struct epoll_event*)loop->events + idx;
  return ev->events & EPOLLIN;
}

static inline bool evloop_is_writable(event_loop_t* loop, int idx) {
  struct epoll_event* ev = (struct epoll_event*)loop->events + idx;
  return ev->events & EPOLLOUT;
}

#endif

/* ============ 工具函数 ============ */

static void* xmalloc(size_t size) {
  void* p = malloc(size);
  if (p) memset(p, 0, size);
  return p;
}

static void set_error(perf_test_t* test, const char* fmt, ...) {
  if (test->err_msg) free(test->err_msg);
  
  va_list ap;
  va_start(ap, fmt);
  int len = vsnprintf(NULL, 0, fmt, ap);
  va_end(ap);
  
  test->err_msg = xmalloc(len + 1);
  if (test->err_msg) {
    va_start(ap, fmt);
    vsnprintf(test->err_msg, len + 1, fmt, ap);
    va_end(ap);
  }
}

static void histogram_init(latency_histogram_t* h, double max_latency_us, int buckets) {
  h->buckets = calloc(buckets, sizeof(uint32_t));
  h->bucket_count = buckets;
  h->bucket_size_us = max_latency_us / buckets;
  h->total_samples = 0;
  h->min_val = max_latency_us;
  h->max_val = 0;
}

static void histogram_add(latency_histogram_t* h, double latency_us) {
  int bucket = (int)(latency_us / h->bucket_size_us);
  if (bucket >= h->bucket_count) bucket = h->bucket_count - 1;
  if (bucket < 0) bucket = 0;
  
  __sync_fetch_and_add(&h->buckets[bucket], 1);
  __sync_fetch_and_add(&h->total_samples, 1);
  
  if (latency_us < h->min_val) h->min_val = latency_us;
  if (latency_us > h->max_val) h->max_val = latency_us;
}

static double histogram_percentile(latency_histogram_t* h, double p) {
  uint64_t target = (uint64_t)(h->total_samples * p / 100.0);
  uint64_t count = 0;
  
  for (int i = 0; i < h->bucket_count; i++) {
    count += h->buckets[i];
    if (count >= target) {
      return (i + 1) * h->bucket_size_us;
    }
  }
  return h->max_val;
}

static size_t ws_build_frame(uint8_t* buf, uint8_t opcode, bool mask, 
                              const uint8_t* payload, size_t payload_len) {
  size_t pos = 0;
  
  buf[pos++] = 0x80 | opcode;
  
  if (payload_len < 126) {
    buf[pos++] = (mask ? 0x80 : 0) | payload_len;
  } else if (payload_len < 65536) {
    buf[pos++] = (mask ? 0x80 : 0) | 126;
    buf[pos++] = (payload_len >> 8) & 0xFF;
    buf[pos++] = payload_len & 0xFF;
  } else {
    buf[pos++] = (mask ? 0x80 : 0) | 127;
    for (int i = 7; i >= 0; i--) {
      buf[pos++] = (payload_len >> (i * 8)) & 0xFF;
    }
  }
  
  if (mask) {
    uint32_t masking_key = (uint32_t)rand();
    buf[pos++] = (masking_key >> 24) & 0xFF;
    buf[pos++] = (masking_key >> 16) & 0xFF;
    buf[pos++] = (masking_key >> 8) & 0xFF;
    buf[pos++] = masking_key & 0xFF;
    
    for (size_t i = 0; i < payload_len; i++) {
      buf[pos + i] = payload[i] ^ ((uint8_t*)&masking_key)[i % 4];
    }
    pos += payload_len;
  } else {
    memcpy(buf + pos, payload, payload_len);
    pos += payload_len;
  }
  
  return pos;
}

static int ws_parse_frame_header(connection_t* conn) {
  if (conn->recv_len < 2) return 0;
  
  uint8_t* p = conn->recv_buf + conn->recv_pos;
  
  conn->ws_opcode = p[0] & 0x0F;
  conn->ws_masked = (p[1] & 0x80) != 0;
  uint64_t payload_len = p[1] & 0x7F;
  
  size_t header_len = 2;
  
  if (payload_len == 126) {
    if (conn->recv_len < 4) return 0;
    payload_len = ((uint64_t)p[2] << 8) | p[3];
    header_len = 4;
  } else if (payload_len == 127) {
    if (conn->recv_len < 10) return 0;
    payload_len = 0;
    for (int i = 0; i < 8; i++) {
      payload_len = (payload_len << 8) | p[2 + i];
    }
    header_len = 10;
  }
  
  if (conn->ws_masked) {
    header_len += 4;
  }
  
  conn->ws_payload_len = payload_len;
  conn->ws_frame_len = header_len;
  
  return 1;
}

static int set_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static int set_nodelay(int fd) {
  int yes = 1;
  return setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes));
}

static int set_sndbuf(int fd, int size) {
  return setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size));
}

static int set_rcvbuf(int fd, int size) {
  return setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
}

static int build_handshake_request(char* buf, size_t bufsize, 
                                    const char* host, const char* path,
                                    const char* origin) {
  unsigned char key_bytes[16];
  for (int i = 0; i < 16; i++) {
    key_bytes[i] = rand() & 0xFF;
  }
  
  static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  char key_b64[25];
  for (int i = 0; i < 16; i += 3) {
    int val = (key_bytes[i] << 16) | (key_bytes[i+1] << 8) | key_bytes[i+2];
    key_b64[i*4/3] = b64[(val >> 18) & 0x3F];
    key_b64[i*4/3+1] = b64[(val >> 12) & 0x3F];
    key_b64[i*4/3+2] = b64[(val >> 6) & 0x3F];
    key_b64[i*4/3+3] = b64[val & 0x3F];
  }
  key_b64[24] = '\0';
  
  int n = snprintf(buf, bufsize,
    "GET %s HTTP/1.1\r\n"
    "Host: %s\r\n"
    "Upgrade: websocket\r\n"
    "Connection: Upgrade\r\n"
    "Sec-WebSocket-Key: %s\r\n"
    "Sec-WebSocket-Version: 13\r\n"
    "%s%s%s"
    "\r\n",
    path, host, key_b64,
    origin ? "Origin: " : "",
    origin ? origin : "",
    origin ? "\r\n" : ""
  );
  
  return n;
}

static connection_t* conn_new(worker_t* worker) {
  connection_t* conn = calloc(1, sizeof(connection_t));
  if (!conn) return NULL;
  
  conn->fd = -1;
  conn->state = CONN_IDLE;
  conn->send_buf = malloc(MAX_MESSAGE_SIZE + 256);
  conn->recv_buf = malloc(MAX_MESSAGE_SIZE + 256);
  conn->latency_samples = malloc(10000 * sizeof(double));
  conn->latency_sample_capacity = 10000;
  
  if (!conn->send_buf || !conn->recv_buf || !conn->latency_samples) {
    free(conn->send_buf);
    free(conn->recv_buf);
    free(conn->latency_samples);
    free(conn);
    return NULL;
  }
  
  conn->id = worker->conn_count++;
  conn->stats = &worker->stats;
  
  return conn;
}

static void conn_free(connection_t* conn) {
  if (conn->fd >= 0) {
    close(conn->fd);
  }
  free(conn->send_buf);
  free(conn->recv_buf);
  free(conn->latency_samples);
  free(conn);
}

static int conn_start_connect(connection_t* conn, perf_test_t* test) {
  conn->fd = socket(AF_INET, SOCK_STREAM, 0);
  if (conn->fd < 0) return -1;
  
  set_nonblocking(conn->fd);
  set_nodelay(conn->fd);
  set_sndbuf(conn->fd, 256 * 1024);
  set_rcvbuf(conn->fd, 256 * 1024);
  
  struct sockaddr_in addr = {
    .sin_family = AF_INET,
    .sin_port = htons(test->port),
  };
  inet_pton(AF_INET, test->host, &addr.sin_addr);
  
  timespec_now(&conn->ts_connect_start);
  
  int rc = connect(conn->fd, (struct sockaddr*)&addr, sizeof(addr));
  if (rc < 0 && errno != EINPROGRESS) {
    return -1;
  }
  
  conn->state = CONN_CONNECTING;
  return 0;
}

static int conn_complete_handshake(connection_t* conn, perf_test_t* test) {
  int so_error;
  socklen_t len = sizeof(so_error);
  getsockopt(conn->fd, SOL_SOCKET, SO_ERROR, &so_error, &len);
  
  if (so_error != 0) {
    conn->state = CONN_ERROR;
    return -1;
  }
  
  timespec_now(&conn->ts_connect_end);
  double connect_ms = timespec_diff_ms(&conn->ts_connect_start, &conn->ts_connect_end);
  
  __sync_fetch_and_add(&conn->stats->connections_success, 1);
  
  conn->stats->connect_time_ms_avg = 
    (conn->stats->connect_time_ms_avg * (conn->stats->connections_success - 1) + connect_ms) 
    / conn->stats->connections_success;
  
  char request[2048];
  int req_len = build_handshake_request(request, sizeof(request), 
                                          test->host, test->path, NULL);
  
  send(conn->fd, request, req_len, MSG_NOSIGNAL);
  
  conn->state = CONN_HANDSHAKE;
  return 0;
}

static void* worker_thread(void* arg) {
  worker_t* worker = arg;
  perf_test_t* test = worker->test;
  
  if (evloop_init(&worker->evloop, worker->max_conns/*worker->max_events*/) < 0) {
    fprintf(stderr, "Worker %d: Failed to create event loop\n", worker->id);
    return NULL;
  }
  
  worker->conns = calloc(worker->max_conns, sizeof(connection_t));
  for (int i = 0; i < worker->max_conns; i++) {
    connection_t* conn = conn_new(worker);
    conn->next = worker->free_list;
    worker->free_list = conn;
  }
  
  int target_conns = worker->max_conns;
  for (int i = 0; i < target_conns && i < 100; i++) {
    connection_t* conn = worker->free_list;
    if (!conn) break;
    worker->free_list = conn->next;
    
    if (conn_start_connect(conn, test) == 0) {
      evloop_add(&worker->evloop, conn->fd, conn, true);
      worker->conns[i] = *conn;
      free(conn);
    }
  }
  
  __sync_fetch_and_add(&test->ready_workers, 1);
  
  while (!test->test_running && !test->interrupted) {
    usleep(1000);
  }
  
  worker->running = true;
  
  while (worker->running && !test->interrupted) {
    int nfds = evloop_wait(&worker->evloop, 10);
    
    for (int i = 0; i < nfds; i++) {
      connection_t* conn = evloop_get_data(&worker->evloop, i);
      bool readable = evloop_is_readable(&worker->evloop, i);
      bool writable = evloop_is_writable(&worker->evloop, i);
      bool error = evloop_is_error(&worker->evloop, i);
      
      if (error) {
        conn->state = CONN_ERROR;
        continue;
      }
      
      switch (conn->state) {
        case CONN_CONNECTING:
          if (writable) {
            conn_complete_handshake(conn, test);
          }
          break;
          
        case CONN_HANDSHAKE:
          conn->state = CONN_OPEN;
          __sync_fetch_and_add(&conn->stats->connections_active, 1);
          break;
          
        case CONN_OPEN:
        case CONN_SENDING:
          if (writable) {
            if (test->mode == MODE_ECHO || test->mode == MODE_THROUGHPUT) {
              uint8_t opcode = test->binary_mode ? WS_OP_BINARY : WS_OP_TEXT;
              size_t frame_len = ws_build_frame(
                conn->send_buf, opcode, test->use_masking,
                (uint8_t*)conn->send_buf + 256, test->message_size
              );
              
              int sent = send(conn->fd, conn->send_buf, frame_len, MSG_NOSIGNAL);
              
              if (sent > 0) {
                conn->messages_sent++;
                conn->bytes_sent += sent;
                __sync_fetch_and_add(&conn->stats->messages_sent, 1);
                __sync_fetch_and_add(&conn->stats->bytes_sent, sent);
                
                timespec_now(&conn->ts_send);
                conn->state = CONN_WAITING;
                
                evloop_mod(&worker->evloop, conn->fd, conn, false);
              }
            }
          }
          break;
          
        case CONN_WAITING:
        case CONN_RECEIVING:
          if (readable) {
            int received = recv(conn->fd, conn->recv_buf + conn->recv_len, 
                                MAX_MESSAGE_SIZE - conn->recv_len, 0);
            
            if (received > 0) {
              conn->recv_len += received;
              conn->bytes_received += received;
              __sync_fetch_and_add(&conn->stats->bytes_received, received);
              
              while (conn->recv_len >= 2) {
                if (!ws_parse_frame_header(conn)) break;
                
                size_t total_frame = conn->ws_frame_len + conn->ws_payload_len;
                if (conn->recv_len < total_frame) break;
                
                if (conn->ws_opcode == WS_OP_TEXT || conn->ws_opcode == WS_OP_BINARY) {
                  conn->messages_received++;
                  __sync_fetch_and_add(&conn->stats->messages_received, 1);
                  
                  struct timespec now;
                  timespec_now(&now);
                  double latency_us = timespec_diff_us(&conn->ts_send, &now);
                  histogram_add(&worker->histogram, latency_us);
                  
                  conn->messages_received++;
                  conn->state = CONN_OPEN;
                  
                  if (test->mode == MODE_THROUGHPUT) {
                    evloop_mod(&worker->evloop, conn->fd, conn, true);
                  }
                }
                
                memmove(conn->recv_buf, conn->recv_buf + total_frame, 
                        conn->recv_len - total_frame);
                conn->recv_len -= total_frame;
              }
            } else if (received == 0 || (received < 0 && errno != EAGAIN)) {
              conn->state = CONN_CLOSED;
            }
          }
          break;
          
        default:
          break;
      }
    }
  }
  
  evloop_close(&worker->evloop);
  worker->running = false;
  return NULL;
}

static void print_usage(const char* prog) {
  printf("WebSocket Performance Tester %s (No TLS)\n\n", WS_PERF_VERSION);
  printf("Usage: %s [options] <url>\n\n", prog);
  printf("Options:\n");
  printf("  -c, --concurrency <n>    Connections (default: %d)\n", DEFAULT_CONCURRENCY);
  printf("  -d, --duration <sec>     Duration (default: %d)\n", DEFAULT_DURATION_SEC);
  printf("  -s, --size <bytes>       Message size (default: %d)\n", DEFAULT_MESSAGE_SIZE);
  printf("  -r, --rate <msg/sec>     Rate limit per conn\n");
  printf("  -m, --mode <mode>        echo/broadcast/ping/throughput\n");
  printf("  -w, --workers <n>        Worker threads\n");
  printf("  -b, --binary             Binary frames\n");
  printf("  -M, --mask               Enable masking\n");
  printf("  -v, --verify             Verify echo\n");
  printf("  -q, --quiet              Quiet mode\n");
  printf("  -h, --help               Show help\n");
  printf("\nNote: TLS (wss://) is not supported in this build.\n");
  printf("      Use ws:// for plain WebSocket connections.\n");
}

int perf_test_init(perf_test_t* test, int argc, char** argv) {
  memset(test, 0, sizeof(*test));
  
  test->duration_sec = DEFAULT_DURATION_SEC;
  test->concurrency = DEFAULT_CONCURRENCY;
  test->message_size = DEFAULT_MESSAGE_SIZE;
  test->mode = MODE_ECHO;
  test->worker_count = sysconf(_SC_NPROCESSORS_ONLN);
  
  static struct option long_opts[] = {
    {"concurrency", required_argument, 0, 'c'},
    {"duration", required_argument, 0, 'd'},
    {"size", required_argument, 0, 's'},
    {"rate", required_argument, 0, 'r'},
    {"mode", required_argument, 0, 'm'},
    {"workers", required_argument, 0, 'w'},
    {"binary", no_argument, 0, 'b'},
    {"mask", no_argument, 0, 'M'},
    {"verify", no_argument, 0, 'v'},
    {"quiet", no_argument, 0, 'q'},
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0}
  };
  
  int opt;
  while ((opt = getopt_long(argc, argv, "c:d:s:r:m:w:bMvqho:", long_opts, NULL)) != -1) {
    switch (opt) {
      case 'c': test->concurrency = atoi(optarg); break;
      case 'd': test->duration_sec = atoi(optarg); break;
      case 's': test->message_size = atoi(optarg); break;
      case 'r': test->message_rate = atoi(optarg); break;
      case 'm':
        if (strcmp(optarg, "echo") == 0) test->mode = MODE_ECHO;
        else if (strcmp(optarg, "broadcast") == 0) test->mode = MODE_BROADCAST;
        else if (strcmp(optarg, "ping") == 0) test->mode = MODE_PING_PONG;
        else if (strcmp(optarg, "throughput") == 0) test->mode = MODE_THROUGHPUT;
        break;
      case 'w': test->worker_count = atoi(optarg); break;
      case 'b': test->binary_mode = true; break;
      case 'M': test->use_masking = true; break;
      case 'v': test->verify_echo = true; break;
      case 'q': break;
      case 'h': print_usage(argv[0]); return -1;
      default: print_usage(argv[0]); return -1;
    }
  }
  
  if (optind >= argc) {
    print_usage(argv[0]);
    return -1;
  }
  
  strncpy(test->url, argv[optind], MAX_URL_LEN - 1);
  
  if (strncmp(test->url, "wss://", 6) == 0) {
    fprintf(stderr, "Error: TLS (wss://) is not supported.\n");
    fprintf(stderr, "Please use ws:// for plain WebSocket connections.\n");
    return -1;
  } else if (strncmp(test->url, "ws://", 5) == 0) {
    test->proto = PROTO_WS;
    sscanf(test->url + 5, "%255[^:/]:%d/%1023s", test->host, &test->port, test->path);
  } else {
    fprintf(stderr, "Invalid URL scheme. Use ws://\n");
    return -1;
  }
  
  if (test->port == 0) {
    test->port = 80;
  }
  
  if (test->concurrency > MAX_CONCURRENCY) {
    fprintf(stderr, "Concurrency too high, max is %d\n", MAX_CONCURRENCY);
    return -1;
  }
  
  test->workers = calloc(test->worker_count, sizeof(worker_t));
  int conns_per_worker = test->concurrency / test->worker_count;
  
  for (int i = 0; i < test->worker_count; i++) {
    worker_t* w = &test->workers[i];
    w->id = i;
    w->test = test;
    w->max_conns = conns_per_worker;
    // w->max_events = conns_per_worker;
    histogram_init(&w->histogram, 100000.0, 1000);
  }
  
  pthread_mutex_init(&test->stats_mutex, NULL);
  pthread_cond_init(&test->start_cond, NULL);
  
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);
  
  return 0;
}

int perf_test_run(perf_test_t* test) {
  printf("WebSocket Performance Test\n");
  printf("==========================\n");
  printf("URL:        %s\n", test->url);
  printf("Mode:       %s\n", 
         test->mode == MODE_ECHO ? "echo" :
         test->mode == MODE_BROADCAST ? "broadcast" :
         test->mode == MODE_PING_PONG ? "ping-pong" : "throughput");
  printf("Connections: %d (%d per worker)\n", test->concurrency, 
         test->concurrency / test->worker_count);
  printf("Duration:   %d seconds\n", test->duration_sec);
  printf("Message:    %d bytes (%s)\n", test->message_size, 
         test->binary_mode ? "binary" : "text");
  printf("Workers:    %d\n\n", test->worker_count);
  
  for (int i = 0; i < test->worker_count; i++) {
    pthread_create(&test->workers[i].thread, NULL, worker_thread, &test->workers[i]);
  }
  
  printf("Establishing connections...");
  fflush(stdout);
  
  while (test->ready_workers < test->worker_count) {
    if (g_interrupted) return -1;
    usleep(10000);
  }
  
  sleep(2);
  printf(" OK\n\n");
  
  test->test_running = true;
  timespec_now(&test->total_stats.start_time);
  
  printf("Testing");
  fflush(stdout);
  
  int elapsed = 0;
  while (elapsed < test->duration_sec && !g_interrupted) {
    sleep(1);
    elapsed++;
    if (elapsed % 5 == 0) {
      printf(".");
      fflush(stdout);
    }
    
    uint64_t active = 0;
    for (int i = 0; i < test->worker_count; i++) {
      active += test->workers[i].stats.connections_active;
    }
    
    if (active == 0 && elapsed > 5) {
      printf("\nWarning: All connections lost!\n");
      break;
    }
  }
  
  printf(" Done\n\n");
  
  test->interrupted = g_interrupted;
  test->test_running = false;
  timespec_now(&test->total_stats.end_time);
  
  for (int i = 0; i < test->worker_count; i++) {
    test->workers[i].should_stop = true;
    pthread_join(test->workers[i].thread, NULL);
  }
  
  test->total_stats.duration_sec = 
    timespec_diff_ms(&test->total_stats.start_time, &test->total_stats.end_time) / 1000.0;
  
  for (int i = 0; i < test->worker_count; i++) {
    worker_t* w = &test->workers[i];
    perf_stats_t* s = &w->stats;
    perf_stats_t* t = &test->total_stats;
    
    t->connections_attempted += s->connections_attempted;
    t->connections_success += s->connections_success;
    t->connections_failed += s->connections_failed;
    t->connections_active += s->connections_active;
    t->messages_sent += s->messages_sent;
    t->messages_received += s->messages_received;
    t->messages_echoed += s->messages_echoed;
    t->bytes_sent += s->bytes_sent;
    t->bytes_received += s->bytes_received;
    
    for (int j = 0; j < w->histogram.bucket_count; j++) {
      test->total_histogram.buckets[j] += w->histogram.buckets[j];
    }
    test->total_histogram.total_samples += w->histogram.total_samples;
  }
  
  test->total_stats.throughput_mbps = 
    (double)(test->total_stats.bytes_sent + test->total_stats.bytes_received) * 8 
    / test->total_stats.duration_sec / 1000000.0;
  
  test->total_stats.msg_rate_send = 
    (double)test->total_stats.messages_sent / test->total_stats.duration_sec;
  
  test->total_stats.msg_rate_recv = 
    (double)test->total_stats.messages_received / test->total_stats.duration_sec;
  
  test->total_histogram.bucket_size_us = 100.0;
  test->total_stats.latency_us_avg = histogram_percentile(&test->total_histogram, 50);
  test->total_stats.latency_us_p50 = histogram_percentile(&test->total_histogram, 50);
  test->total_stats.latency_us_p95 = histogram_percentile(&test->total_histogram, 95);
  test->total_stats.latency_us_p99 = histogram_percentile(&test->total_histogram, 99);
  test->total_stats.latency_us_min = test->total_histogram.min_val;
  test->total_stats.latency_us_max = test->total_histogram.max_val;
  
  return 0;
}

void perf_test_report(perf_test_t* test) {
  perf_stats_t* s = &test->total_stats;
  
  printf("╔══════════════════════════════════════════════════════════════╗\n");
  printf("║                    TEST RESULTS                              ║\n");
  printf("╠══════════════════════════════════════════════════════════════╣\n");
  printf("║ Duration:           %10.2f seconds                        ║\n", s->duration_sec);
  printf("║                                                              ║\n");
  printf("║ CONNECTIONS                                                  ║\n");
  printf("║   Attempted:        %10lu                               ║\n", s->connections_attempted);
  printf("║   Successful:       %10lu (%.1f%%)                      ║\n", 
         s->connections_success, 
         s->connections_attempted > 0 ? (100.0 * s->connections_success / s->connections_attempted) : 0);
  printf("║   Failed:           %10lu                               ║\n", s->connections_failed);
  printf("║   Avg Connect:      %10.2f ms                           ║\n", s->connect_time_ms_avg);
  printf("║                                                              ║\n");
  printf("║ MESSAGES                                                     ║\n");
  printf("║   Sent:             %10lu (%.0f/sec)                    ║\n", 
         s->messages_sent, s->msg_rate_send);
  printf("║   Received:         %10lu (%.0f/sec)                    ║\n", 
         s->messages_received, s->msg_rate_recv);
  printf("║   Echoed:           %10lu                               ║\n", s->messages_echoed);
  printf("║   Loss:             %10lu (%.2f%%)                        ║\n",
         s->messages_sent - s->messages_echoed,
         s->messages_sent > 0 ? (100.0 * (s->messages_sent - s->messages_echoed) / s->messages_sent) : 0);
  printf("║                                                              ║\n");
  printf("║ THROUGHPUT                                                   ║\n");
  printf("║   Total:            %10.2f Mbps                         ║\n", s->throughput_mbps);
  printf("║   Sent:             %10.2f MB                           ║\n", s->bytes_sent / 1048576.0);
  printf("║   Received:         %10.2f MB                           ║\n", s->bytes_received / 1048576.0);
  printf("║                                                              ║\n");
  printf("║ LATENCY (microseconds)                                      ║\n");
  printf("║   Min:              %10.1f                              ║\n", s->latency_us_min);
  printf("║   Avg:              %10.1f                              ║\n", s->latency_us_avg);
  printf("║   p50:              %10.1f                              ║\n", s->latency_us_p50);
  printf("║   p95:              %10.1f                              ║\n", s->latency_us_p95);
  printf("║   p99:              %10.1f                              ║\n", s->latency_us_p99);
  printf("║   Max:              %10.1f                              ║\n", s->latency_us_max);
  printf("╚══════════════════════════════════════════════════════════════╝\n");
}

void perf_test_cleanup(perf_test_t* test) {
  for (int i = 0; i < test->worker_count; i++) {
    free(test->workers[i].histogram.buckets);
  }
  free(test->workers);
  
  pthread_mutex_destroy(&test->stats_mutex);
  pthread_cond_destroy(&test->start_cond);
}

int main(int argc, char** argv) {
  perf_test_t test;
  
  if (perf_test_init(&test, argc, argv) != 0) {
    return 1;
  }
  
  if (perf_test_run(&test) != 0) {
    perf_test_cleanup(&test);
    return 1;
  }
  
  perf_test_report(&test);
  perf_test_cleanup(&test);
  
  return 0;
}