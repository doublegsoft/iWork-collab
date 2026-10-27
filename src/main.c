/*
**    ▄▄▄▄  ▄▄▄  ▄▄▄▄                   
** ▀▀ ▀███  ███  ███▀            ▄▄     
** ██  ███  ███  ███ ▄███▄ ████▄ ██ ▄█▀ 
** ██  ███▄▄███▄▄███ ██ ██ ██ ▀▀ ████   
** ██▄  ▀████▀████▀  ▀███▀ ██    ██ ▀█▄ 
*/
#include <libwebsockets.h>
#include <uv.h>
#include <string.h>
#include <stdio.h>

#include <together-net.h>

static struct lws_context* context = NULL;

static int callback_minimal(struct lws* wsi, enum lws_callback_reasons reason,
                            void* user, void* in, size_t len) {
  switch (reason) {
    case LWS_CALLBACK_ESTABLISHED:
      printf("[WS] Connection established\n");
      break;

    case LWS_CALLBACK_RECEIVE: {
      printf("[WS] Received data: %.*s\n", (int)len, (char*)in);
      
      // Libwebsockets requires data to be padded with LWS_PRE bytes at the front
      unsigned char buf[LWS_PRE + 128];
      unsigned char* p = &buf[LWS_PRE];
      
      // Format a simple reply
      int n = snprintf((char*)p, 128, "Server received: %.*s", (int)len, (char*)in);
      
      // Write data back to the client
      lws_write(wsi, p, n, LWS_WRITE_TEXT);
      break;
    }

    case LWS_CALLBACK_CLOSED:
      printf("[WS] Connection closed\n");
      break;

    default:
      break;
  }
  return 0;
}

// 2. Define the protocols supported by this server
static struct lws_protocols protocols[] = {
  {
    "iWork-protocol", // Protocol name (used by clients connecting)
    callback_minimal,   // Callback function
    0,                  // Per-session data size
    0,                  // Maximum frame size
  },
  { NULL, NULL, 0, 0 }  /* terminator */
};

// 3. Graceful shutdown handler
static void signal_cb(uv_signal_t* handle, int signum) {
  printf("\nInterrupt received. Stopping...\n");
  uv_stop(handle->loop);
}


#define BUFFER_SIZE 4096

/**
 * @brief 执行 Bash Shell 命令的 Net Handler 回调函数
 * 
 * @param req       请求帧（包含要执行的 shell 命令）
 * @param resp      响应帧（用于写回 shell 执行结果）
 * @param user_data 用户上下文数据（可选）
 * @return int      0: 成功, -1: 失败
 */
int tgr_net_bash_handler(const tgr_frame_p req, tgr_frame_p resp, void* user_data) {
  if (req == NULL || resp == NULL) {
    return -1;
  }

  // 1. 从 req 中获取命令字符串
  // 注意：这里假设 req->data 是以 '\0' 结尾的字符串命令，如 "ls -l /tmp"
  // 如果你的 tgr_frame 提供了 API，比如 tgr_frame_get_data(req)，请替换这里
  // const char* cmd = (const char*)req->data; 
  // if (cmd == NULL || strlen(cmd) == 0) {
  //   // 错误处理：空命令
  //   return -1;
  // }

  // 2. 为了同时捕获 stderr，将命令包装为重定向错误输出
  // 组装格式类似于: "bash -c 'your_command' 2>&1"
  char full_cmd[BUFFER_SIZE];
  sprintf(full_cmd, "bash -c 'cd /export/local/works/doublegsoft.me/myhotkey/03.Development/paws-cli/ && ./script/paws-deepseek-codewiki.sh ' 2>&1");

  // 3. 执行 popen
  FILE* fp = popen(full_cmd, "r");
  if (fp == NULL) {
    perror("popen failed");
    return -1;
  }

  // 4. 读取命令输出并写入 resp
  char chunk[512];
  
  // 方式 A：如果 resp 提供了追加数据的接口（推荐，防止输出过大）
  // 例如：tgr_frame_append(resp, chunk, bytes_read);
  
  // 方式 B：手动填充 resp 的缓冲区（假设 resp->data 预分配了空间）
  size_t total_read = 0;
  // char* output_ptr = (char*)resp->data;
  size_t max_resp_size = BUFFER_SIZE; // 假设允许的最大响应长度

  // while (fgets(chunk, sizeof(chunk), fp) != NULL) {
  //   size_t chunk_len = strlen(chunk);
    
  //   // 防止缓冲区溢出
  //   if (total_read + chunk_len < max_resp_size) {
  //     memcpy(output_ptr + total_read, chunk, chunk_len);
  //     total_read += chunk_len;
  //   } else {
  //     // 输出截断处理
  //     break; 
  //   }
  // }
  
  // 补齐字符串结束符
  // if (total_read < max_resp_size) {
  //   output_ptr[total_read] = '\0';
  // } else {
  //   output_ptr[max_resp_size - 1] = '\0';
  // }

  // 设置响应大小 (根据实际 tgr_frame 字段进行调整)
  // resp->size = total_read; 

  // 5. 关闭管道并获取退出状态码
  int status = pclose(fp);
  int exit_code = -1;
  if (WIFEXITED(status)) {
    exit_code = WEXITSTATUS(status);
  }

  // 如果 shell 执行返回非 0，你可以选择返回错误，或者将状态写入 resp header
  return (exit_code == 0) ? 0 : -1;
}

int main(void) 
{
  // struct lws_context_creation_info info;
  // uv_loop_t* loop = uv_default_loop();
  
  // // Setup graceful exit on CTRL+C
  // uv_signal_t sigint;
  // uv_signal_init(loop, &sigint);
  // uv_signal_start(&sigint, signal_cb, SIGINT);

  // memset(&info, 0, sizeof info);
  // info.port = 8848;
  // info.protocols = protocols;
  
  // info.options = LWS_SERVER_OPTION_LIBUV; 
  
  // // Provide the existing libuv loop to LWS (Foreign loop)
  // void* foreign_loops[1];
  // foreign_loops[0] = loop;
  // info.foreign_loops = foreign_loops;

  // context = lws_create_context(&info);
  // if (!context) {
  //   fprintf(stderr, "libwebsockets init failed\n");
  //   return -1;
  // }

  // printf("WebSocket Server started on ws://localhost:%d\n", info.port);
  // printf("Press CTRL+C to stop.\n");

  // uv_run(loop, UV_RUN_DEFAULT);

  // lws_context_destroy(context);
  // uv_loop_close(loop);

  // return 0;
  tgr_net_serve(5447, tgr_net_bash_handler, NULL);
}