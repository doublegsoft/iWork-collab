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
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <time.h>

#ifdef __APPLE__
#include <sys/event.h>
#define USE_KQUEUE
#else
#include <sys/epoll.h>
#define USE_EPOLL
#endif

#define BUF_SIZE 65536

static volatile int running = 1;

void sig_handler(int sig) {
  running = 0;
}

int set_nonblock(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int ws_handshake(int fd, const char* host, const char* path) {
  char req[1024];
  snprintf(req, sizeof(req),
    "GET %s HTTP/1.1\r\n"
    "Host: %s\r\n"
    "Upgrade: websocket\r\n"
    "Connection: Upgrade\r\n"
    "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
    "Sec-WebSocket-Version: 13\r\n"
    "\r\n",
    path, host);
  
  return send(fd, req, strlen(req), 0);
}

int ws_send(int fd, const char* msg, int len) {
  unsigned char frame[BUF_SIZE];
  int pos = 0;
  
  frame[pos++] = 0x81;  /* FIN + text */
  
  if (len < 126) {
    frame[pos++] = len;
  } else {
    frame[pos++] = 126;
    frame[pos++] = (len >> 8) & 0xFF;
    frame[pos++] = len & 0xFF;
  }
  
  memcpy(frame + pos, msg, len);
  pos += len;
  
  return send(fd, frame, pos, 0);
}

int ws_recv(int fd, char* buf, int max) {
  unsigned char header[4];
  int n = recv(fd, (char*)header, 2, MSG_PEEK);
  if (n < 2) return 0;
  
  int opcode = header[0] & 0x0F;
  int masked = (header[1] & 0x80) != 0;
  int len = header[1] & 0x7F;
  int hlen = 2;
  
  if (len == 126) {
    n = recv(fd, (char*)header, 4, MSG_PEEK);
    if (n < 4) return 0;
    len = (header[2] << 8) | header[3];
    hlen = 4;
  }
  
  if (opcode == 0x8) return -1;  /* close */
  
  /* discard header */
  char tmp[8];
  recv(fd, tmp, hlen, 0);
  
  if (masked) {
    unsigned char mask[4];
    recv(fd, (char*)mask, 4, 0);
    n = recv(fd, buf, len > max ? max : len, 0);
    for (int i = 0; i < n; i++) buf[i] ^= mask[i % 4];
  } else {
    n = recv(fd, buf, len > max ? max : len, 0);
  }
  
  return n;
}

int connect_ws(const char* host, int port) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) return -1;
  
  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  
  struct hostent* he = gethostbyname(host);
  if (!he) {
    close(fd);
    return -1;
  }
  memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);
  
  if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    close(fd);
    return -1;
  }
  
  return fd;
}

int main(int argc, char** argv) {
  if (argc < 2) {
    printf("Usage: %s ws://host:port/path [connections] [duration]\n", argv[0]);
    printf("  Example: %s ws://localhost:8080/echo 100 30\n", argv[0]);
    return 1;
  }
  
  signal(SIGINT, sig_handler);
  
  /* parse url */
  char host[256] = {0};
  int port = 80;
  char path[256] = "/";
  
  if (sscanf(argv[1], "ws://%255[^:/]:%d/%255s", host, &port, path) < 2) {
    sscanf(argv[1], "ws://%255[^/]/%255s", host, path);
  }
  
  int conns = argc > 2 ? atoi(argv[2]) : 1;
  int duration = argc > 3 ? atoi(argv[3]) : 10;
  
  printf("Testing ws://%s:%d/%s\n", host, port, path);
  printf("Connections: %d, Duration: %ds\n\n", conns, duration);
  
  /* create connections */
  int* fds = calloc(conns, sizeof(int));
  int connected = 0;
  
  for (int i = 0; i < conns; i++) {
    fds[i] = connect_ws(host, port);
    if (fds[i] < 0) continue;
    
    set_nonblock(fds[i]);
    ws_handshake(fds[i], host, path);
    connected++;
  }
  
  printf("Connected: %d/%d\n", connected, conns);
  
  /* wait for handshake responses */
  usleep(100000);
  
  /* setup event loop */
#ifdef USE_KQUEUE
  int kq = kqueue();
  struct kevent* evs = calloc(conns * 2, sizeof(struct kevent));
#else
  int ep = epoll_create1(0);
  struct epoll_event* evs = calloc(conns, sizeof(struct epoll_event));
#endif
  
  char* buf = calloc(BUF_SIZE, 1);
  long total_sent = 0, total_recv = 0;
  int messages = 0;
  
  time_t start = time(NULL);
  
  while (running && time(NULL) - start < duration) {
    /* send on all connections */
    for (int i = 0; i < conns; i++) {
      if (fds[i] < 0) continue;
      
      char msg[256];
      snprintf(msg, sizeof(msg), "hello %d", messages++);
      int n = ws_send(fds[i], msg, strlen(msg));
      if (n > 0) total_sent += n;
    }
    
    /* poll for responses */
#ifdef USE_KQUEUE
    struct timespec ts = {0, 10000000};  /* 10ms */
    int nfds = kevent(kq, NULL, 0, evs, conns, &ts);
#else
    int nfds = epoll_wait(ep, evs, conns, 10);
#endif
    
    for (int i = 0; i < conns; i++) {
      if (fds[i] < 0) continue;
      
      int n = ws_recv(fds[i], buf, BUF_SIZE);
      if (n > 0) {
        total_recv += n;
        buf[n] = 0;
      } else if (n < 0) {
        close(fds[i]);
        fds[i] = -1;
      }
    }
    
    usleep(1000);  /* 1ms throttle */
  }
  
  time_t elapsed = time(NULL) - start;
  
  printf("\n=== Results ===\n");
  printf("Duration: %lds\n", elapsed);
  printf("Sent: %ld bytes (%.1f KB/s)\n", total_sent, total_sent / 1024.0 / elapsed);
  printf("Recv: %ld bytes (%.1f KB/s)\n", total_recv, total_recv / 1024.0 / elapsed);
  printf("Messages: %d (%.0f msg/s)\n", messages, (double)messages / elapsed);
  
  /* cleanup */
  for (int i = 0; i < conns; i++) {
    if (fds[i] >= 0) close(fds[i]);
  }
  free(fds);
  free(evs);
  free(buf);
  
#ifdef USE_KQUEUE
  close(kq);
#else
  close(ep);
#endif
  
  return 0;
}