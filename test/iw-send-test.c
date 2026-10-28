#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define TGR_MAGIC 0x5447
#define TGR_HEADER_WIRE_SIZE (64 + 2 + 1 + 1 + 16 + 2 + 4 + 4 + 4)

typedef struct tgr_frame_s  tgr_frame_t;
typedef        tgr_frame_t* tgr_frame_p;

struct tgr_frame_s {
  char type_name[64];
  uint16_t magic;
  uint8_t version;
  uint8_t message_type;
  char sender_host[16];
  uint16_t sender_port;
  int sequence_id;
  int error;
  int payload_len;
  uint8_t* payload;
};

static void serialize_frame_header(const tgr_frame_p frame, uint8_t* buf) {
  uint8_t* p = buf;

  uint16_t magic_net = htons(frame->magic);
  memcpy(p, &magic_net, 2);
  p += 2;

  *p = frame->version;
  p += 1;

  *p = frame->message_type;
  p += 1;

  memcpy(p, frame->sender_host, 16);
  p += 16;

  uint16_t port_net = htons(frame->sender_port);
  memcpy(p, &port_net, 2);
  p += 2;

  uint32_t seq_net = htonl((uint32_t)frame->sequence_id);
  memcpy(p, &seq_net, 4);
  p += 4;

  uint32_t err_net = htonl((uint32_t)frame->error);
  memcpy(p, &err_net, 4);
  p += 4;

  uint32_t len_net = htonl((uint32_t)frame->payload_len);
  memcpy(p, &len_net, 4);
  p += 4;
}

static int write_all(int fd, const void* buf, size_t len) {
  size_t total_sent = 0;
  const uint8_t* p = (const uint8_t*)buf;

  while (total_sent < len) {
    ssize_t ret = write(fd, p + total_sent, len - total_sent);
    if (ret < 0) {
      perror("write failed");
      return -1;
    }
    total_sent += ret;
  }
  return 0;
}

int tgr_frame_send_to(const char* target_ip, uint16_t target_port, const tgr_frame_p frame) {
  if (!target_ip || !frame) {
    return -1;
  }

  int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_fd < 0) {
    perror("socket creation failed");
    return -1;
  }

  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(target_port);

  if (inet_pton(AF_INET, target_ip, &server_addr.sin_addr) <= 0) {
    perror("invalid IP address");
    close(sock_fd);
    return -1;
  }

  if (connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
    perror("connect failed");
    close(sock_fd);
    return -1;
  }

  uint8_t header_buf[TGR_HEADER_WIRE_SIZE];
  serialize_frame_header(frame, header_buf);

  if (write_all(sock_fd, header_buf, TGR_HEADER_WIRE_SIZE) < 0) {
    close(sock_fd);
    return -1;
  }

  if (frame->payload_len > 0 && frame->payload != NULL) {
    if (write_all(sock_fd, frame->payload, frame->payload_len) < 0) {
      close(sock_fd);
      return -1;
    }
  }

  close(sock_fd);
  return 0;
}

int main(void) {
  tgr_frame_t req_frame;
  memset(&req_frame, 0, sizeof(tgr_frame_t));

  const char* bash_cmd = "ls -la /tmp";

  strncpy(req_frame.type_name, "BashCommandRequest", sizeof(req_frame.type_name) - 1);
  req_frame.magic = TGR_MAGIC;
  req_frame.version = 1;
  req_frame.message_type = 0x01;
  strncpy(req_frame.sender_host, "127.0.0.1", sizeof(req_frame.sender_host) - 1);
  req_frame.sender_port = 8888;
  req_frame.sequence_id = 1001;
  req_frame.error = 0;
  req_frame.payload_len = strlen(bash_cmd) + 1;
  req_frame.payload = (uint8_t*)bash_cmd;

  const char* target_ip = "10.12.100.120";
  uint16_t target_port = 5447;

  printf("Sending frame to %s:%d...\n", target_ip, target_port);
  if (tgr_frame_send_to(target_ip, target_port, &req_frame) == 0) {
    printf("Frame sent successfully!\n");
  } else {
    printf("Failed to send frame.\n");
  }

  return 0;
}