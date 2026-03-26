/*
** Copyright 2026 doublegsoft.ai
** 
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the “Software”), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
** 
** The above copyright notice and this permission notice shall be included in all
** copies or substantial portions of the Software.
** 
** THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
** THE SOFTWARE.
*/
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "iWork-pkt-codec.h"

iw_prompt_p 
iw_prompt_decode(const unsigned char* bytes, 
                 size_t buf_len)
{
  iw_prompt_p ret = iw_prompt_init();
  size_t offset = 0;

  if (buf_len < sizeof(iw_prompt_t)) {
    return NULL; 
  }

  // 类型：提示词
  memcpy((int*)&ret->magic, bytes + offset, 4);
  offset += 4;
  // 版本
  memcpy(ret->version, bytes + offset, 2);
  offset += 2;
  // 请求
  memcpy((long*)&ret->request, bytes + offset, 8);
  offset += 4;
  // 类型：提示词
  memcpy(ret->type, bytes + offset, 2);
  offset += 2;
  // 指令长度
  memcpy((int*)&ret->length, bytes + offset, 4);
  offset += 4;
  // 文本长度
  memcpy((int*)&ret->text_length, bytes + offset, 4);
  offset += 4;
  // 文本内容
  ret->text = (char*)malloc(sizeof(char) * (buf_len - offset));
  memcpy(ret->text, bytes + offset, buf_len - offset);
  offset += buf_len - offset;
  // 文件个数
  memcpy((int*)&ret->file_count, bytes + offset, 4);
  offset += 4;
  // 各个文件长度
  // 文件内容
  ret->files = (char*)malloc(sizeof(char) * (buf_len - offset));
  memcpy(ret->files, bytes + offset, buf_len - offset);
  offset += buf_len - offset;

  return ret;
}

iw_compilation_p 
iw_compilation_decode(const unsigned char* bytes, 
                      size_t buf_len)
{
  iw_compilation_p ret = iw_compilation_init();
  size_t offset = 0;

  if (buf_len < sizeof(iw_compilation_t)) {
    return NULL; 
  }

  // 指令长度
  memcpy((int*)&ret->len, bytes + offset, 4);
  offset += 4;
  // 指令类型
  memcpy(ret->type, bytes + offset, 2);
  offset += 2;
  // 编程语言
  memcpy(ret->language, bytes + offset, 2);
  offset += 2;
  // 代码长度
  memcpy((int*)&ret->src_len, bytes + offset, 4);
  offset += 4;
  // 代码
  ret->source = (char*)malloc(sizeof(char) * (buf_len - offset));
  memcpy(ret->source, bytes + offset, buf_len - offset);
  offset += buf_len - offset;

  return ret;
}

iw_build_p 
iw_build_decode(const unsigned char* bytes, 
                size_t buf_len)
{
  iw_build_p ret = iw_build_init();
  size_t offset = 0;

  if (buf_len < sizeof(iw_build_t)) {
    return NULL; 
  }

  // 指令长度
  memcpy((int*)&ret->length, bytes + offset, 4);
  offset += 4;
  // 指令类型
  memcpy(ret->type, bytes + offset, 2);
  offset += 2;
  // 编程语言
  memcpy(ret->build_tool, bytes + offset, 2);
  offset += 2;
  // 项目路径
  memcpy(ret->project_path, bytes + offset, 200);
  offset += 200;

  return ret;
}

iw_generation_p 
iw_generation_decode(const unsigned char* bytes, 
                     size_t buf_len)
{
  iw_generation_p ret = iw_generation_init();
  size_t offset = 0;

  if (buf_len < sizeof(iw_generation_t)) {
    return NULL; 
  }

  // 包长度
  memcpy((int*)&ret->length, bytes + offset, 4);
  offset += 4;
  // 指令类型
  memcpy(ret->type, bytes + offset, 2);
  offset += 2;
  // 编程语言
  memcpy(ret->file_type, bytes + offset, 2);
  offset += 2;
  // 项目路径
  memcpy(ret->source, bytes + offset, 200);
  offset += 200;

  return ret;
}

iw_preview_p 
iw_preview_decode(const unsigned char* bytes, 
                  size_t buf_len)
{
  iw_preview_p ret = iw_preview_init();
  size_t offset = 0;

  if (buf_len < sizeof(iw_preview_t)) {
    return NULL; 
  }

  // 包长度
  memcpy((int*)&ret->length, bytes + offset, 4);
  offset += 4;
  // 指令类型
  memcpy(ret->type, bytes + offset, 2);
  offset += 2;
  // 编程语言
  memcpy(ret->file_type, bytes + offset, 2);
  offset += 2;
  // 项目路径
  memcpy(ret->source, bytes + offset, 200);
  offset += 200;

  return ret;
}
