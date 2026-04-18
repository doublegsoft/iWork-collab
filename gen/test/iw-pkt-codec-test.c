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

#include "iw-pkt-codec.h"

void
iw_prompt_decode_test(void) 
{
  printf("Running test: iw_prompt_decode...\n");

  iw_prompt_t source_obj;
  memset(&source_obj, 0, sizeof(iw_prompt_t));
  iw_prompt_p prompt = &source_obj;

  // 初始化属性: magic
  memset(&prompt->magic, 0xAA, 4);
  // 初始化属性: type
  memcpy(prompt->type, "PT", sizeof(prompt->type));  
  // 初始化属性: version
  memcpy(prompt->version, "01", sizeof(prompt->version));  
  // 初始化属性: request
  memset(&prompt->request, 0xAA, 8);
  // 初始化属性: text_length
  prompt->text_length = 100; 
  // 初始化属性: text
  prompt->text = (char*)malloc(prompt->text_length);  
  memset(prompt->text, 0xBB, prompt->text_length);
  // 初始化属性: file_count
  prompt->file_count = 3;
  // 初始化属性: file_lens
  prompt->file_lens = malloc(3 * sizeof(int)); // 根据实际类型强转
  prompt->file_lens[0] = 100;
  prompt->file_lens[1] = 200;
  prompt->file_lens[2] = 300;    
  // 初始化属性: files
  size_t files_block_bytes = 0;
  for (int i = 0; i < 3; i++)
    files_block_bytes += prompt->file_lens[i];
  prompt->files = malloc(files_block_bytes);
  memset(prompt->files, 0xCC, files_block_bytes);
  
  // 注意：为了防止未初始化的动态数组导致越界，计数器 (countedName) 保持为 0，
  // 这样 Encode 时只会编码基础字段。如需测试变长数组，可在此手动 malloc 并设置计数值。

  unsigned char* encoded_bytes = NULL;
  size_t encoded_size = 0;

  // 调用对应的 encode 函数，生成合法的 byte buffer
  iw_prompt_encode(prompt, &encoded_bytes, &encoded_size);
  assert(encoded_bytes != NULL);

  // 为了通过 decode 内部 "buf_len < sizeof(struct)" 的安全检查：
  // 如果实际编码长度小于结构体大小（因为指针本身占空间但不序列化指针地址），需补齐 buffer 长度
  size_t test_buf_len = encoded_size > sizeof(iw_prompt_t) ? encoded_size : sizeof(iw_prompt_t);

  unsigned char* decode_buffer = (unsigned char*)calloc(1, test_buf_len);
  memcpy(decode_buffer, encoded_bytes, encoded_size);

  // ==========================================
  // 2. Act: 执行 Decode 函数
  // ==========================================
  iw_prompt_p decoded_obj = iw_prompt_decode(decode_buffer, test_buf_len);

  // ==========================================
  // 3. Assert: 验证反序列化结果是否与源数据一致
  // ==========================================
  assert(decoded_obj != NULL);
  
  // 验证固定长度字段: magic
  assert(memcmp(&source_obj.magic, &decoded_obj->magic, 4) == 0);
  // 验证固定长度字段: type
  assert(memcmp(source_obj.type, decoded_obj->type, 2) == 0);
  // 验证固定长度字段: version
  assert(memcmp(source_obj.version, decoded_obj->version, 2) == 0);
  // 验证固定长度字段: request
  assert(memcmp(&source_obj.request, &decoded_obj->request, 8) == 0);
  // 验证固定长度字段: text_length
  assert(memcmp(&source_obj.text_length, &decoded_obj->text_length, 4) == 0);
  // 验证固定长度字段: text
  assert(memcmp(source_obj.text, decoded_obj->text, prompt->text_length) == 0);
  // 验证固定长度字段: file_count
  assert(memcmp(&source_obj.file_count, &decoded_obj->file_count, 4) == 0);
  // 验证固定长度字段: file_lens
  assert(memcmp(source_obj.file_lens, decoded_obj->file_lens, 4 * prompt->file_count) == 0);

  // ==========================================
  // 4. Cleanup: 释放内存防泄漏
  // ==========================================
  free(encoded_bytes);
  free(decode_buffer);
  
  // 释放 Decode 动态生成的对象 (需确保内部动态分配的字段也被 free)
  if (decoded_obj != NULL) {
    // TODO: 如果解码时给变长字段分配了内存，需在此一并 free
    free(decoded_obj); 
  }

  printf("Test passed: iw_prompt_decode\n");
}

void
iw_prompt_encode_test(void) 
{
  printf("Running test: iw_prompt_encode...\n");
  iw_prompt_t dummy_obj;
  memset(&dummy_obj, 0, sizeof(iw_prompt_t));
  iw_prompt_p prompt = &dummy_obj;

  size_t expected_total_size = 0;

  // 初始化属性: magic
  memset(&prompt->magic, 0xAA, 4);
  expected_total_size += 4;
  // 初始化属性: type
  memcpy(prompt->type, "PT", sizeof(prompt->type));  
  expected_total_size += sizeof(prompt->type);  
  // 初始化属性: version
  memcpy(prompt->version, "01", sizeof(prompt->version));  
  expected_total_size += sizeof(prompt->version);  
  // 初始化属性: request
  memset(&prompt->request, 0xAA, 8);
  expected_total_size += 8;
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

  free(prompt->file_lens);
  free(prompt->files);
  free(encoded_bytes);

  printf("Test passed: iw_prompt_encode\n");
}

void
iw_coding_decode_test(void) 
{
  printf("Running test: iw_coding_decode...\n");

  iw_coding_t source_obj;
  memset(&source_obj, 0, sizeof(iw_coding_t));
  iw_coding_p coding = &source_obj;

  // 初始化属性: magic
  memset(&coding->magic, 0xAA, 4);
  // 初始化属性: type
  memcpy(coding->type, "CD", sizeof(coding->type));  
  // 初始化属性: version
  memcpy(coding->version, "01", sizeof(coding->version));  
  // 初始化属性: request
  memset(&coding->request, 0xAA, 8);
  // 初始化属性: text_length
  coding->text_length = 100; 
  // 初始化属性: text
  coding->text = (char*)malloc(coding->text_length);  
  memset(coding->text, 0xBB, coding->text_length);
  
  // 注意：为了防止未初始化的动态数组导致越界，计数器 (countedName) 保持为 0，
  // 这样 Encode 时只会编码基础字段。如需测试变长数组，可在此手动 malloc 并设置计数值。

  unsigned char* encoded_bytes = NULL;
  size_t encoded_size = 0;

  // 调用对应的 encode 函数，生成合法的 byte buffer
  iw_coding_encode(coding, &encoded_bytes, &encoded_size);
  assert(encoded_bytes != NULL);

  // 为了通过 decode 内部 "buf_len < sizeof(struct)" 的安全检查：
  // 如果实际编码长度小于结构体大小（因为指针本身占空间但不序列化指针地址），需补齐 buffer 长度
  size_t test_buf_len = encoded_size > sizeof(iw_coding_t) ? encoded_size : sizeof(iw_coding_t);

  unsigned char* decode_buffer = (unsigned char*)calloc(1, test_buf_len);
  memcpy(decode_buffer, encoded_bytes, encoded_size);

  // ==========================================
  // 2. Act: 执行 Decode 函数
  // ==========================================
  iw_coding_p decoded_obj = iw_coding_decode(decode_buffer, test_buf_len);

  // ==========================================
  // 3. Assert: 验证反序列化结果是否与源数据一致
  // ==========================================
  assert(decoded_obj != NULL);
  
  // 验证固定长度字段: magic
  assert(memcmp(&source_obj.magic, &decoded_obj->magic, 4) == 0);
  // 验证固定长度字段: type
  assert(memcmp(source_obj.type, decoded_obj->type, 2) == 0);
  // 验证固定长度字段: version
  assert(memcmp(source_obj.version, decoded_obj->version, 2) == 0);
  // 验证固定长度字段: request
  assert(memcmp(&source_obj.request, &decoded_obj->request, 8) == 0);
  // 验证固定长度字段: text_length
  assert(memcmp(&source_obj.text_length, &decoded_obj->text_length, 4) == 0);
  // 验证固定长度字段: text
  assert(memcmp(source_obj.text, decoded_obj->text, coding->text_length) == 0);

  // ==========================================
  // 4. Cleanup: 释放内存防泄漏
  // ==========================================
  free(encoded_bytes);
  free(decode_buffer);
  
  // 释放 Decode 动态生成的对象 (需确保内部动态分配的字段也被 free)
  if (decoded_obj != NULL) {
    // TODO: 如果解码时给变长字段分配了内存，需在此一并 free
    free(decoded_obj); 
  }

  printf("Test passed: iw_coding_decode\n");
}

void
iw_coding_encode_test(void) 
{
  printf("Running test: iw_coding_encode...\n");
  iw_coding_t dummy_obj;
  memset(&dummy_obj, 0, sizeof(iw_coding_t));
  iw_coding_p coding = &dummy_obj;

  size_t expected_total_size = 0;

  // 初始化属性: magic
  memset(&coding->magic, 0xAA, 4);
  expected_total_size += 4;
  // 初始化属性: type
  memcpy(coding->type, "CD", sizeof(coding->type));  
  expected_total_size += sizeof(coding->type);  
  // 初始化属性: version
  memcpy(coding->version, "01", sizeof(coding->version));  
  expected_total_size += sizeof(coding->version);  
  // 初始化属性: request
  memset(&coding->request, 0xAA, 8);
  expected_total_size += 8;
  // 初始化属性: text_length
  coding->text_length = 100; 
  expected_total_size += 4;  
  // 初始化属性: text
  coding->text = (char*)malloc(coding->text_length);  
  memset(coding->text, 0xBB, coding->text_length);
  expected_total_size += coding->text_length;
  unsigned char* encoded_bytes = NULL;
  size_t encoded_size = 0;

  iw_coding_encode(coding, &encoded_bytes, &encoded_size);

  assert(encoded_bytes != NULL);
  assert(encoded_size == expected_total_size);

  free(encoded_bytes);

  printf("Test passed: iw_coding_encode\n");
}

void
iw_compilation_decode_test(void) 
{
  printf("Running test: iw_compilation_decode...\n");

  iw_compilation_t source_obj;
  memset(&source_obj, 0, sizeof(iw_compilation_t));
  iw_compilation_p compilation = &source_obj;

  // 初始化属性: magic
  memset(&compilation->magic, 0xAA, 4);
  // 初始化属性: type
  memcpy(compilation->type, "CX", sizeof(compilation->type));  
  // 初始化属性: language
  memset(&compilation->language, 0xAA, 2);
  // 初始化属性: src_len
  compilation->src_len = 100; 
  // 初始化属性: source
  compilation->source = (char*)malloc(compilation->src_len);  
  memset(compilation->source, 0xBB, compilation->src_len);
  
  // 注意：为了防止未初始化的动态数组导致越界，计数器 (countedName) 保持为 0，
  // 这样 Encode 时只会编码基础字段。如需测试变长数组，可在此手动 malloc 并设置计数值。

  unsigned char* encoded_bytes = NULL;
  size_t encoded_size = 0;

  // 调用对应的 encode 函数，生成合法的 byte buffer
  iw_compilation_encode(compilation, &encoded_bytes, &encoded_size);
  assert(encoded_bytes != NULL);

  // 为了通过 decode 内部 "buf_len < sizeof(struct)" 的安全检查：
  // 如果实际编码长度小于结构体大小（因为指针本身占空间但不序列化指针地址），需补齐 buffer 长度
  size_t test_buf_len = encoded_size > sizeof(iw_compilation_t) ? encoded_size : sizeof(iw_compilation_t);

  unsigned char* decode_buffer = (unsigned char*)calloc(1, test_buf_len);
  memcpy(decode_buffer, encoded_bytes, encoded_size);

  // ==========================================
  // 2. Act: 执行 Decode 函数
  // ==========================================
  iw_compilation_p decoded_obj = iw_compilation_decode(decode_buffer, test_buf_len);

  // ==========================================
  // 3. Assert: 验证反序列化结果是否与源数据一致
  // ==========================================
  assert(decoded_obj != NULL);
  
  // 验证固定长度字段: magic
  assert(memcmp(&source_obj.magic, &decoded_obj->magic, 4) == 0);
  // 验证固定长度字段: type
  assert(memcmp(source_obj.type, decoded_obj->type, 2) == 0);
  // 验证固定长度字段: language
  assert(memcmp(source_obj.language, decoded_obj->language, 2) == 0);
  // 验证固定长度字段: src_len
  assert(memcmp(&source_obj.src_len, &decoded_obj->src_len, 4) == 0);
  // 验证固定长度字段: source
  assert(memcmp(source_obj.source, decoded_obj->source, compilation->src_len) == 0);

  // ==========================================
  // 4. Cleanup: 释放内存防泄漏
  // ==========================================
  free(encoded_bytes);
  free(decode_buffer);
  
  // 释放 Decode 动态生成的对象 (需确保内部动态分配的字段也被 free)
  if (decoded_obj != NULL) {
    // TODO: 如果解码时给变长字段分配了内存，需在此一并 free
    free(decoded_obj); 
  }

  printf("Test passed: iw_compilation_decode\n");
}

void
iw_compilation_encode_test(void) 
{
  printf("Running test: iw_compilation_encode...\n");
  iw_compilation_t dummy_obj;
  memset(&dummy_obj, 0, sizeof(iw_compilation_t));
  iw_compilation_p compilation = &dummy_obj;

  size_t expected_total_size = 0;

  // 初始化属性: magic
  memset(&compilation->magic, 0xAA, 4);
  expected_total_size += 4;
  // 初始化属性: type
  memcpy(compilation->type, "CX", sizeof(compilation->type));  
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

  free(encoded_bytes);

  printf("Test passed: iw_compilation_encode\n");
}

void
iw_build_decode_test(void) 
{
  printf("Running test: iw_build_decode...\n");

  iw_build_t source_obj;
  memset(&source_obj, 0, sizeof(iw_build_t));
  iw_build_p build = &source_obj;

  // 初始化属性: magic
  int magic_dflval = 287454020;
  memcpy(&build->magic, &magic_dflval, sizeof(build->magic));  
  // 初始化属性: type
  memcpy(build->type, "BD", sizeof(build->type));  
  // 初始化属性: build_tool
  memset(&build->build_tool, 0xAA, 2);
  // 初始化属性: project_path
  memset(&build->project_path, 0xAA, 200);
  
  // 注意：为了防止未初始化的动态数组导致越界，计数器 (countedName) 保持为 0，
  // 这样 Encode 时只会编码基础字段。如需测试变长数组，可在此手动 malloc 并设置计数值。

  unsigned char* encoded_bytes = NULL;
  size_t encoded_size = 0;

  // 调用对应的 encode 函数，生成合法的 byte buffer
  iw_build_encode(build, &encoded_bytes, &encoded_size);
  assert(encoded_bytes != NULL);

  // 为了通过 decode 内部 "buf_len < sizeof(struct)" 的安全检查：
  // 如果实际编码长度小于结构体大小（因为指针本身占空间但不序列化指针地址），需补齐 buffer 长度
  size_t test_buf_len = encoded_size > sizeof(iw_build_t) ? encoded_size : sizeof(iw_build_t);

  unsigned char* decode_buffer = (unsigned char*)calloc(1, test_buf_len);
  memcpy(decode_buffer, encoded_bytes, encoded_size);

  // ==========================================
  // 2. Act: 执行 Decode 函数
  // ==========================================
  iw_build_p decoded_obj = iw_build_decode(decode_buffer, test_buf_len);

  // ==========================================
  // 3. Assert: 验证反序列化结果是否与源数据一致
  // ==========================================
  assert(decoded_obj != NULL);
  
  // 验证固定长度字段: magic
  assert(memcmp(&source_obj.magic, &decoded_obj->magic, 4) == 0);
  // 验证固定长度字段: type
  assert(memcmp(source_obj.type, decoded_obj->type, 2) == 0);
  // 验证固定长度字段: build_tool
  assert(memcmp(source_obj.build_tool, decoded_obj->build_tool, 2) == 0);
  // 验证固定长度字段: project_path
  assert(memcmp(source_obj.project_path, decoded_obj->project_path, 200) == 0);

  // ==========================================
  // 4. Cleanup: 释放内存防泄漏
  // ==========================================
  free(encoded_bytes);
  free(decode_buffer);
  
  // 释放 Decode 动态生成的对象 (需确保内部动态分配的字段也被 free)
  if (decoded_obj != NULL) {
    // TODO: 如果解码时给变长字段分配了内存，需在此一并 free
    free(decoded_obj); 
  }

  printf("Test passed: iw_build_decode\n");
}

void
iw_build_encode_test(void) 
{
  printf("Running test: iw_build_encode...\n");
  iw_build_t dummy_obj;
  memset(&dummy_obj, 0, sizeof(iw_build_t));
  iw_build_p build = &dummy_obj;

  size_t expected_total_size = 0;

  // 初始化属性: magic
  int magic_dflval = 287454020;
  memcpy(&build->magic, &magic_dflval, sizeof(build->magic));  
  expected_total_size += sizeof(build->magic);  
  // 初始化属性: type
  memcpy(build->type, "BD", sizeof(build->type));  
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

  free(encoded_bytes);

  printf("Test passed: iw_build_encode\n");
}

void
iw_preview_decode_test(void) 
{
  printf("Running test: iw_preview_decode...\n");

  iw_preview_t source_obj;
  memset(&source_obj, 0, sizeof(iw_preview_t));
  iw_preview_p preview = &source_obj;

  // 初始化属性: magic
  int magic_dflval = 287454020;
  memcpy(&preview->magic, &magic_dflval, sizeof(preview->magic));  
  // 初始化属性: type
  memcpy(preview->type, "PV", sizeof(preview->type));  
  // 初始化属性: file_type
  memset(&preview->file_type, 0xAA, 2);
  // 初始化属性: source
  memset(&preview->source, 0xAA, 200);
  
  // 注意：为了防止未初始化的动态数组导致越界，计数器 (countedName) 保持为 0，
  // 这样 Encode 时只会编码基础字段。如需测试变长数组，可在此手动 malloc 并设置计数值。

  unsigned char* encoded_bytes = NULL;
  size_t encoded_size = 0;

  // 调用对应的 encode 函数，生成合法的 byte buffer
  iw_preview_encode(preview, &encoded_bytes, &encoded_size);
  assert(encoded_bytes != NULL);

  // 为了通过 decode 内部 "buf_len < sizeof(struct)" 的安全检查：
  // 如果实际编码长度小于结构体大小（因为指针本身占空间但不序列化指针地址），需补齐 buffer 长度
  size_t test_buf_len = encoded_size > sizeof(iw_preview_t) ? encoded_size : sizeof(iw_preview_t);

  unsigned char* decode_buffer = (unsigned char*)calloc(1, test_buf_len);
  memcpy(decode_buffer, encoded_bytes, encoded_size);

  // ==========================================
  // 2. Act: 执行 Decode 函数
  // ==========================================
  iw_preview_p decoded_obj = iw_preview_decode(decode_buffer, test_buf_len);

  // ==========================================
  // 3. Assert: 验证反序列化结果是否与源数据一致
  // ==========================================
  assert(decoded_obj != NULL);
  
  // 验证固定长度字段: magic
  assert(memcmp(&source_obj.magic, &decoded_obj->magic, 4) == 0);
  // 验证固定长度字段: type
  assert(memcmp(source_obj.type, decoded_obj->type, 2) == 0);
  // 验证固定长度字段: file_type
  assert(memcmp(source_obj.file_type, decoded_obj->file_type, 2) == 0);
  // 验证固定长度字段: source
  assert(memcmp(source_obj.source, decoded_obj->source, 200) == 0);

  // ==========================================
  // 4. Cleanup: 释放内存防泄漏
  // ==========================================
  free(encoded_bytes);
  free(decode_buffer);
  
  // 释放 Decode 动态生成的对象 (需确保内部动态分配的字段也被 free)
  if (decoded_obj != NULL) {
    // TODO: 如果解码时给变长字段分配了内存，需在此一并 free
    free(decoded_obj); 
  }

  printf("Test passed: iw_preview_decode\n");
}

void
iw_preview_encode_test(void) 
{
  printf("Running test: iw_preview_encode...\n");
  iw_preview_t dummy_obj;
  memset(&dummy_obj, 0, sizeof(iw_preview_t));
  iw_preview_p preview = &dummy_obj;

  size_t expected_total_size = 0;

  // 初始化属性: magic
  int magic_dflval = 287454020;
  memcpy(&preview->magic, &magic_dflval, sizeof(preview->magic));  
  expected_total_size += sizeof(preview->magic);  
  // 初始化属性: type
  memcpy(preview->type, "PV", sizeof(preview->type));  
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

  free(encoded_bytes);

  printf("Test passed: iw_preview_encode\n");
}

void
iw_generation_decode_test(void) 
{
  printf("Running test: iw_generation_decode...\n");

  iw_generation_t source_obj;
  memset(&source_obj, 0, sizeof(iw_generation_t));
  iw_generation_p generation = &source_obj;

  // 初始化属性: magic
  memset(&generation->magic, 0xAA, 4);
  // 初始化属性: request
  memset(&generation->request, 0xAA, 8);
  // 初始化属性: text_length
  generation->text_length = 100; 
  // 初始化属性: text
  generation->text = (char*)malloc(generation->text_length);  
  memset(generation->text, 0xBB, generation->text_length);
  
  // 注意：为了防止未初始化的动态数组导致越界，计数器 (countedName) 保持为 0，
  // 这样 Encode 时只会编码基础字段。如需测试变长数组，可在此手动 malloc 并设置计数值。

  unsigned char* encoded_bytes = NULL;
  size_t encoded_size = 0;

  // 调用对应的 encode 函数，生成合法的 byte buffer
  iw_generation_encode(generation, &encoded_bytes, &encoded_size);
  assert(encoded_bytes != NULL);

  // 为了通过 decode 内部 "buf_len < sizeof(struct)" 的安全检查：
  // 如果实际编码长度小于结构体大小（因为指针本身占空间但不序列化指针地址），需补齐 buffer 长度
  size_t test_buf_len = encoded_size > sizeof(iw_generation_t) ? encoded_size : sizeof(iw_generation_t);

  unsigned char* decode_buffer = (unsigned char*)calloc(1, test_buf_len);
  memcpy(decode_buffer, encoded_bytes, encoded_size);

  // ==========================================
  // 2. Act: 执行 Decode 函数
  // ==========================================
  iw_generation_p decoded_obj = iw_generation_decode(decode_buffer, test_buf_len);

  // ==========================================
  // 3. Assert: 验证反序列化结果是否与源数据一致
  // ==========================================
  assert(decoded_obj != NULL);
  
  // 验证固定长度字段: magic
  assert(memcmp(&source_obj.magic, &decoded_obj->magic, 4) == 0);
  // 验证固定长度字段: request
  assert(memcmp(&source_obj.request, &decoded_obj->request, 8) == 0);
  // 验证固定长度字段: text_length
  assert(memcmp(&source_obj.text_length, &decoded_obj->text_length, 4) == 0);
  // 验证固定长度字段: text
  assert(memcmp(source_obj.text, decoded_obj->text, generation->text_length) == 0);

  // ==========================================
  // 4. Cleanup: 释放内存防泄漏
  // ==========================================
  free(encoded_bytes);
  free(decode_buffer);
  
  // 释放 Decode 动态生成的对象 (需确保内部动态分配的字段也被 free)
  if (decoded_obj != NULL) {
    // TODO: 如果解码时给变长字段分配了内存，需在此一并 free
    free(decoded_obj); 
  }

  printf("Test passed: iw_generation_decode\n");
}

void
iw_generation_encode_test(void) 
{
  printf("Running test: iw_generation_encode...\n");
  iw_generation_t dummy_obj;
  memset(&dummy_obj, 0, sizeof(iw_generation_t));
  iw_generation_p generation = &dummy_obj;

  size_t expected_total_size = 0;

  // 初始化属性: magic
  memset(&generation->magic, 0xAA, 4);
  expected_total_size += 4;
  // 初始化属性: request
  memset(&generation->request, 0xAA, 8);
  expected_total_size += 8;
  // 初始化属性: text_length
  generation->text_length = 100; 
  expected_total_size += 4;  
  // 初始化属性: text
  generation->text = (char*)malloc(generation->text_length);  
  memset(generation->text, 0xBB, generation->text_length);
  expected_total_size += generation->text_length;
  unsigned char* encoded_bytes = NULL;
  size_t encoded_size = 0;

  iw_generation_encode(generation, &encoded_bytes, &encoded_size);

  assert(encoded_bytes != NULL);
  assert(encoded_size == expected_total_size);

  free(encoded_bytes);

  printf("Test passed: iw_generation_encode\n");
}

int main(int argc, char* argv[])
{
  iw_prompt_decode_test();
  iw_prompt_encode_test();
  iw_coding_decode_test();
  iw_coding_encode_test();
  iw_compilation_decode_test();
  iw_compilation_encode_test();
  iw_build_decode_test();
  iw_build_encode_test();
  iw_preview_decode_test();
  iw_preview_encode_test();
  iw_generation_decode_test();
  iw_generation_encode_test();
  return 0;
}
