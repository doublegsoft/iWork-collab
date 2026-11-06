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

/**
 * 读取本地文件到内存
 * @param filepath 文件路径
 * @param out_size 传出参数，存储文件字节数
 * @return 动态分配的文件内容缓冲区指针，读取失败返回 NULL (调用者需负责 free)
 */
static uint8_t* read_file_to_buffer(const char* filepath, size_t* out_size) {
  FILE* fp = fopen(filepath, "rb");
  if (!fp) {
    perror("fopen error");
    return NULL;
  }

  /* 获取文件大小 */
  if (fseek(fp, 0, SEEK_END) != 0) {
    perror("fseek error");
    fclose(fp);
    return NULL;
  }
  long file_len = ftell(fp);
  if (file_len < 0) {
    perror("ftell error");
    fclose(fp);
    return NULL;
  }
  rewind(fp);

  /* 分配内存（多预留 1 字节防止空文件 malloc(0) 未定义行为） */
  uint8_t* buffer = (uint8_t*)malloc(file_len + 1);
  if (!buffer) {
    fprintf(stderr, "malloc failed for file buffer (%ld bytes)\n", file_len);
    fclose(fp);
    return NULL;
  }

  size_t read_bytes = fread(buffer, 1, file_len, fp);
  if (read_bytes != (size_t)file_len) {
    fprintf(stderr, "read error: expected %ld bytes, got %zu\n", file_len, read_bytes);
    free(buffer);
    fclose(fp);
    return NULL;
  }

  fclose(fp);
  *out_size = read_bytes;
  return buffer;
}

int main(int argc, char* argv[]) {
  /* 1. 支持从命令行参数传入文件路径，或者使用默认文件 */
  const char* file_path = "./test.txt";
  if (argc > 1) {
    file_path = argv[1];
  }

  /* 2. 读取文件到内存 */
  size_t file_size = 0;
  uint8_t* file_content = read_file_to_buffer(file_path, &file_size);
  if (!file_content) {
    fprintf(stderr, "Failed to read file: %s\n", file_path);
    return 1;
  }
  printf("Successfully read file '%s', size: %zu bytes\n", file_path, file_size);

  /* 3. 构造请求帧 */
  tgr_frame_t req_frame;
  tgr_frame_p resp_frame = NULL;
  memset(&req_frame, 0, sizeof(tgr_frame_t));

  /* 根据业务调整 type_name，例如传输文件可设为 FileTransferRequest */
  strncpy(req_frame.type_name, "FileTransferRequest", sizeof(req_frame.type_name) - 1);
  req_frame.magic = TGR_MAGIC;
  req_frame.version = 1;
  req_frame.message_type = 0x01;
  strncpy(req_frame.sender_host, "127.0.0.1", sizeof(req_frame.sender_host) - 1);
  req_frame.sender_port = 8888;
  req_frame.sequence_id = 1001;
  req_frame.error = 0;

  /* 设置文件载荷及长度 */
  req_frame.payload_len = (int)file_size;
  req_frame.payload = file_content;

  const char* target_ip = "10.12.100.120";
  uint16_t target_port = 5447;

  printf("Sending frame with file to %s:%d...\n", target_ip, target_port);

  /* 4. 发送网络帧 */
  if (tgr_net_send(target_ip, target_port, &req_frame, &resp_frame) == 0) {
    printf("Frame sent successfully!\n");
    /* 如果有响应帧，在此处处理 resp_frame */
  } else {
    printf("Failed to send frame.\n");
  }

  /* 5. 释放本地读取文件的内存 */
  free(file_content);

  return 0;
}