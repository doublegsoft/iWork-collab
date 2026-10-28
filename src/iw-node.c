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


#define BUFFER_SIZE 4096

/*！
** @brief 执行 Bash Shell 命令的 Net Handler 回调函数
** 
** @param req       请求帧（包含要执行的 shell 命令）
** @param resp      响应帧（用于写回 shell 执行结果）
** @param user_data 用户上下文数据（可选）
** @return int      0: 成功, -1: 失败
*/
int tgr_net_bash_handler(const tgr_frame_p req, tgr_frame_p resp, void* user_data) {
  if (req == NULL || resp == NULL) {
    return -1;
  }

  // 2. 为了同时捕获 stderr，将命令包装为重定向错误输出
  // 组装格式类似于: "bash -c 'your_command' 2>&1"
  char full_cmd[BUFFER_SIZE];
  sprintf(full_cmd, "cd /export/local/works/doublegsoft.me/myhotkey/03.Development/paws-cli/ && ./script/paws-deepseek-codewiki.sh > /dev/null 2>&1");
  
  // 3. 执行 popen
  FILE* fp = popen(full_cmd, "r");
  if (fp == NULL) {
    perror("popen failed");
    return -1;
  }

  // 4. 读取命令输出并写入 resp
  // char chunk[512];
  
  // size_t total_read = 0;
  // size_t max_resp_size = BUFFER_SIZE; // 假设允许的最大响应长度

  // while (fgets(chunk, sizeof(chunk), fp) != NULL) {
  //   size_t chunk_len = strlen(chunk);
  //   fputs(chunk, stdout);
  //   // 防止缓冲区溢出
  //   if (total_read + chunk_len < max_resp_size) {
  //     total_read += chunk_len;
  //   } else {
  //     // 输出截断处理
  //     break; 
  //   }
  // }

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
  tgr_net_serve(5447, tgr_net_bash_handler, NULL);
}
