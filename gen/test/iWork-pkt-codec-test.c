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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "iWork-pkt-codec.h"

void
iw_prompt_encode_test(void) 
{
  printf("Running test: iw_prompt_encode...\n");
  iw_prompt_t dummy_obj;
  memset(&dummy_obj, 0, sizeof(iw_prompt_t));
  iw_prompt_p prompt = &dummy_obj;

  size_t expected_total_size = 0;

  // 初始化属性: magic
  int magic_dflval = 1769947755;
  memcpy(&prompt->magic, &magic_dflval, sizeof(prompt->magic));  
  expected_total_size += sizeof(prompt->magic);  

  // 初始化属性: version
  memcpy(&prompt->version, "01", sizeof(prompt->version));  
  expected_total_size += sizeof(prompt->version);  

  // 初始化属性: request
  memset(&prompt->request, 0xAA, 8);
  expected_total_size += 8;

  // 初始化属性: type
  memcpy(&prompt->type, "PT", sizeof(prompt->type));  
  expected_total_size += sizeof(prompt->type);  

  // 初始化属性: length
  memset(&prompt->length, 0xAA, 4);
  expected_total_size += 4;

  // 初始化属性: text_length
  prompt->text_length = 100; 
  expected_total_size += 4;  

  // 初始化属性: text
  prompt->text = (char*)malloc(prompt->text_length);  
  memset(prompt->text, 0xBB, prompt->text_length);
  expected_total_size += prompt->text_length;

  // 初始化属性: file_count
  prompt->file_count = 3;
  expected_total_size += 4;

  // 初始化属性: file_lens
  prompt->file_lens = malloc(3 * sizeof(int)); // 根据实际类型强转
  prompt->file_lens[0] = 100;
  prompt->file_lens[1] = 200;
  prompt->file_lens[2] = 300;    
  expected_total_size += 3 * sizeof(int);

  // 初始化属性: files
  size_t files_block_bytes = 0;
  for (int i = 0; i < 3; i++)
    files_block_bytes += prompt->file_lens[i];
  prompt->files = malloc(files_block_bytes);
  memset(prompt->files, 0xCC, files_block_bytes);

  expected_total_size += files_block_bytes;

  unsigned char* encoded_bytes = NULL;
  size_t encoded_size = 0;

  iw_prompt_encode(prompt, &encoded_bytes, &encoded_size);

  assert(encoded_bytes != NULL);
  assert(encoded_size == expected_total_size);

  // ==========================================
  // 4. Cleanup: 释放内存防泄漏
  // ==========================================
  free(prompt->file_lens);
  free(prompt->files);
  free(encoded_bytes);

  printf("Test passed: iw_prompt_encode\n");
}

void
iw_compilation_encode_test(void) 
{
  printf("Running test: iw_compilation_encode...\n");
  iw_compilation_t dummy_obj;
  memset(&dummy_obj, 0, sizeof(iw_compilation_t));
  iw_compilation_p compilation = &dummy_obj;

  size_t expected_total_size = 0;

  // 初始化属性: len
  memset(&compilation->len, 0xAA, 4);
  expected_total_size += 4;

  // 初始化属性: type
  memcpy(&compilation->type, "CX", sizeof(compilation->type));  
  expected_total_size += sizeof(compilation->type);  

  // 初始化属性: language
  memset(&compilation->language, 0xAA, 2);
  expected_total_size += 2;

  // 初始化属性: src_len
  compilation->src_len = 100; 
  expected_total_size += 4;  

  // 初始化属性: source
  compilation->source = (char*)malloc(compilation->src_len);  
  memset(compilation->source, 0xBB, compilation->src_len);
  expected_total_size += compilation->src_len;

  unsigned char* encoded_bytes = NULL;
  size_t encoded_size = 0;

  iw_compilation_encode(compilation, &encoded_bytes, &encoded_size);

  assert(encoded_bytes != NULL);
  assert(encoded_size == expected_total_size);

  // ==========================================
  // 4. Cleanup: 释放内存防泄漏
  // ==========================================
  free(encoded_bytes);

  printf("Test passed: iw_compilation_encode\n");
}

void
iw_build_encode_test(void) 
{
  printf("Running test: iw_build_encode...\n");
  iw_build_t dummy_obj;
  memset(&dummy_obj, 0, sizeof(iw_build_t));
  iw_build_p build = &dummy_obj;

  size_t expected_total_size = 0;

  // 初始化属性: length
  memset(&build->length, 0xAA, 4);
  expected_total_size += 4;

  // 初始化属性: type
  memcpy(&build->type, "BD", sizeof(build->type));  
  expected_total_size += sizeof(build->type);  

  // 初始化属性: build_tool
  memset(&build->build_tool, 0xAA, 2);
  expected_total_size += 2;

  // 初始化属性: project_path
  memset(&build->project_path, 0xAA, 200);
  expected_total_size += 200;

  unsigned char* encoded_bytes = NULL;
  size_t encoded_size = 0;

  iw_build_encode(build, &encoded_bytes, &encoded_size);

  assert(encoded_bytes != NULL);
  assert(encoded_size == expected_total_size);

  // ==========================================
  // 4. Cleanup: 释放内存防泄漏
  // ==========================================
  free(encoded_bytes);

  printf("Test passed: iw_build_encode\n");
}

void
iw_generation_encode_test(void) 
{
  printf("Running test: iw_generation_encode...\n");
  iw_generation_t dummy_obj;
  memset(&dummy_obj, 0, sizeof(iw_generation_t));
  iw_generation_p generation = &dummy_obj;

  size_t expected_total_size = 0;

  // 初始化属性: length
  memset(&generation->length, 0xAA, 4);
  expected_total_size += 4;

  // 初始化属性: type
  memcpy(&generation->type, "GN", sizeof(generation->type));  
  expected_total_size += sizeof(generation->type);  

  // 初始化属性: file_type
  memset(&generation->file_type, 0xAA, 2);
  expected_total_size += 2;

  // 初始化属性: source
  memset(&generation->source, 0xAA, 200);
  expected_total_size += 200;

  unsigned char* encoded_bytes = NULL;
  size_t encoded_size = 0;

  iw_generation_encode(generation, &encoded_bytes, &encoded_size);

  assert(encoded_bytes != NULL);
  assert(encoded_size == expected_total_size);

  // ==========================================
  // 4. Cleanup: 释放内存防泄漏
  // ==========================================
  free(encoded_bytes);

  printf("Test passed: iw_generation_encode\n");
}

void
iw_preview_encode_test(void) 
{
  printf("Running test: iw_preview_encode...\n");
  iw_preview_t dummy_obj;
  memset(&dummy_obj, 0, sizeof(iw_preview_t));
  iw_preview_p preview = &dummy_obj;

  size_t expected_total_size = 0;

  // 初始化属性: length
  memset(&preview->length, 0xAA, 4);
  expected_total_size += 4;

  // 初始化属性: type
  memcpy(&preview->type, "BD", sizeof(preview->type));  
  expected_total_size += sizeof(preview->type);  

  // 初始化属性: file_type
  memset(&preview->file_type, 0xAA, 2);
  expected_total_size += 2;

  // 初始化属性: source
  memset(&preview->source, 0xAA, 200);
  expected_total_size += 200;

  unsigned char* encoded_bytes = NULL;
  size_t encoded_size = 0;

  iw_preview_encode(preview, &encoded_bytes, &encoded_size);

  assert(encoded_bytes != NULL);
  assert(encoded_size == expected_total_size);

  // ==========================================
  // 4. Cleanup: 释放内存防泄漏
  // ==========================================
  free(encoded_bytes);

  printf("Test passed: iw_preview_encode\n");
}

int main(int argc, char* argv[])
{
  iw_prompt_encode_test();
  iw_compilation_encode_test();
  iw_build_encode_test();
  iw_generation_encode_test();
  iw_preview_encode_test();
  return 0;
}
