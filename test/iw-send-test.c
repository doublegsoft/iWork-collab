/*
**    ▄▄▄▄  ▄▄▄  ▄▄▄▄                   
** ▀▀ ▀███  ███  ███▀            ▄▄     
** ██  ███  ███  ███ ▄███▄ ████▄ ██ ▄█▀ 
** ██  ███▄▄███▄▄███ ██ ██ ██ ▀▀ ████   
** ██▄  ▀████▀████▀  ▀███▀ ██    ██ ▀█▄ 
*/
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <together-error.h>
#include <together-poco.h>
#include <together-codec.h>  
#include <together-net.h>  

#define TGR_MAGIC 0x5447

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

int main(void) {
  tgr_frame_t req_frame;
  tgr_frame_p resp_frame;
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

  const char* target_ip = "10.12.100.75";
  uint16_t target_port = 5447;

  printf("Sending frame to %s:%d...\n", target_ip, target_port);

  if (tgr_net_send(target_ip, target_port, &req_frame, &resp_frame) == 0) {
    printf("Frame sent successfully!\n");
  } else {
    printf("Failed to send frame.\n");
  }

  return 0;
}