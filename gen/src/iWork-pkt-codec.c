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
  size_t block_bytes = 0;

  // magic
  memcpy((void*)&ret->magic, bytes + offset, 4);
  offset += 4;
  // version
  memcpy((void*)ret->version, bytes + offset, 2);
  offset += 2;
  // request
  memcpy((void*)&ret->request, bytes + offset, 8);
  offset += 8;
  // type
  memcpy((void*)ret->type, bytes + offset, 2);
  offset += 2;
  // length
  memcpy((void*)&ret->length, bytes + offset, 4);
  offset += 4;
  // text_length
  memcpy((void*)&ret->text_length, bytes + offset, 4);
  offset += 4;
  // text
  ret->text = (char*)malloc(ret->text_length);  
  memcpy((void*)ret->text, bytes + offset, ret->text_length);
  offset += ret->text_length;
  // file_count
  memcpy((void*)&ret->file_count, bytes + offset, 4);
  offset += 4;
  // file_lens
  ret->file_lens = (int*)malloc(4 * ret->file_count);
  memcpy((void*)ret->file_lens, bytes + offset, 4 * ret->file_count);
  offset += 4 * ret->file_count;
  block_bytes = 0;    
  for (int i = 0; i < ret->file_count; i++) 
    block_bytes += ret->file_lens[i];  
  ret->files = (char*)malloc(block_bytes);
  memcpy((void*)ret->files, bytes + offset, block_bytes);
  offset += block_bytes;  

  return ret;
}

void
iw_prompt_encode(const iw_prompt_p prompt, 
                 unsigned char** bytes,
                 size_t* size)
{
  size_t offset = 0;
  size_t total_bytes = 0;
  size_t block_bytes = 0;
  // magic    
  total_bytes += 4; 
  // version    
  total_bytes += 2; 
  // request    
  total_bytes += 8; 
  // type    
  total_bytes += 2; 
  // length    
  total_bytes += 4; 
  // text_length    
  total_bytes += 4; 
  // text    
  total_bytes += prompt->text_length; 
  // file_count    
  total_bytes += 4; 
  // file_lens    
  total_bytes += 4 * prompt->file_count; 
  // files    
  for (int i = 0; i < prompt->file_count; i++) 
    total_bytes += prompt->file_lens[i];  
  *size = total_bytes;
  *bytes = (unsigned char*)malloc(total_bytes);
  // magic
  memcpy((*bytes) + offset, &prompt->magic, 4);
  offset += 4;
  // version
  memcpy((*bytes) + offset, prompt->version, 2);
  offset += 2;
  // request
  memcpy((*bytes) + offset, &prompt->request, 8);
  offset += 8;
  // type
  memcpy((*bytes) + offset, prompt->type, 2);
  offset += 2;
  // length
  memcpy((*bytes) + offset, &prompt->length, 4);
  offset += 4;
  // text_length
  memcpy((*bytes) + offset, &prompt->text_length, 4);
  offset += 4;
  // text
  memcpy((*bytes) + offset, prompt->text, prompt->text_length);
  offset += prompt->text_length;
  // file_count
  memcpy((*bytes) + offset, &prompt->file_count, 4);
  offset += 4;
  // file_lens
  memcpy((*bytes) + offset, prompt->file_lens, 4 * prompt->file_count);
  offset += 4 * prompt->file_count;
  // files
  block_bytes = 0;    
  for (int i = 0; i < prompt->file_count; i++) 
    block_bytes += prompt->file_lens[i];  
  memcpy((*bytes) + offset, prompt->files, block_bytes);
  offset += block_bytes;  
}

iw_compilation_p 
iw_compilation_decode(const unsigned char* bytes, 
                      size_t buf_len)
{
  iw_compilation_p ret = iw_compilation_init();
  size_t offset = 0;
  size_t block_bytes = 0;

  // len
  memcpy((void*)&ret->len, bytes + offset, 4);
  offset += 4;
  // type
  memcpy((void*)ret->type, bytes + offset, 2);
  offset += 2;
  // language
  memcpy((void*)ret->language, bytes + offset, 2);
  offset += 2;
  // src_len
  memcpy((void*)&ret->src_len, bytes + offset, 4);
  offset += 4;
  // source
  ret->source = (char*)malloc(ret->src_len);  
  memcpy((void*)ret->source, bytes + offset, ret->src_len);
  offset += ret->src_len;

  return ret;
}

void
iw_compilation_encode(const iw_compilation_p compilation, 
                      unsigned char** bytes,
                      size_t* size)
{
  size_t offset = 0;
  size_t total_bytes = 0;
  size_t block_bytes = 0;
  // len    
  total_bytes += 4; 
  // type    
  total_bytes += 2; 
  // language    
  total_bytes += 2; 
  // src_len    
  total_bytes += 4; 
  // source    
  total_bytes += compilation->src_len; 
  *size = total_bytes;
  *bytes = (unsigned char*)malloc(total_bytes);
  // len
  memcpy((*bytes) + offset, &compilation->len, 4);
  offset += 4;
  // type
  memcpy((*bytes) + offset, compilation->type, 2);
  offset += 2;
  // language
  memcpy((*bytes) + offset, compilation->language, 2);
  offset += 2;
  // src_len
  memcpy((*bytes) + offset, &compilation->src_len, 4);
  offset += 4;
  // source
  memcpy((*bytes) + offset, compilation->source, compilation->src_len);
  offset += compilation->src_len;
}

iw_build_p 
iw_build_decode(const unsigned char* bytes, 
                size_t buf_len)
{
  iw_build_p ret = iw_build_init();
  size_t offset = 0;
  size_t block_bytes = 0;

  // length
  memcpy((void*)&ret->length, bytes + offset, 4);
  offset += 4;
  // type
  memcpy((void*)ret->type, bytes + offset, 2);
  offset += 2;
  // build_tool
  memcpy((void*)ret->build_tool, bytes + offset, 2);
  offset += 2;
  // project_path
  memcpy((void*)ret->project_path, bytes + offset, 200);
  offset += 200;

  return ret;
}

void
iw_build_encode(const iw_build_p build, 
                unsigned char** bytes,
                size_t* size)
{
  size_t offset = 0;
  size_t total_bytes = 0;
  size_t block_bytes = 0;
  // length    
  total_bytes += 4; 
  // type    
  total_bytes += 2; 
  // build_tool    
  total_bytes += 2; 
  // project_path    
  total_bytes += 200; 
  *size = total_bytes;
  *bytes = (unsigned char*)malloc(total_bytes);
  // length
  memcpy((*bytes) + offset, &build->length, 4);
  offset += 4;
  // type
  memcpy((*bytes) + offset, build->type, 2);
  offset += 2;
  // build_tool
  memcpy((*bytes) + offset, build->build_tool, 2);
  offset += 2;
  // project_path
  memcpy((*bytes) + offset, build->project_path, 200);
  offset += 200;
}

iw_generation_p 
iw_generation_decode(const unsigned char* bytes, 
                     size_t buf_len)
{
  iw_generation_p ret = iw_generation_init();
  size_t offset = 0;
  size_t block_bytes = 0;

  // length
  memcpy((void*)&ret->length, bytes + offset, 4);
  offset += 4;
  // type
  memcpy((void*)ret->type, bytes + offset, 2);
  offset += 2;
  // file_type
  memcpy((void*)ret->file_type, bytes + offset, 2);
  offset += 2;
  // source
  memcpy((void*)ret->source, bytes + offset, 200);
  offset += 200;

  return ret;
}

void
iw_generation_encode(const iw_generation_p generation, 
                     unsigned char** bytes,
                     size_t* size)
{
  size_t offset = 0;
  size_t total_bytes = 0;
  size_t block_bytes = 0;
  // length    
  total_bytes += 4; 
  // type    
  total_bytes += 2; 
  // file_type    
  total_bytes += 2; 
  // source    
  total_bytes += 200; 
  *size = total_bytes;
  *bytes = (unsigned char*)malloc(total_bytes);
  // length
  memcpy((*bytes) + offset, &generation->length, 4);
  offset += 4;
  // type
  memcpy((*bytes) + offset, generation->type, 2);
  offset += 2;
  // file_type
  memcpy((*bytes) + offset, generation->file_type, 2);
  offset += 2;
  // source
  memcpy((*bytes) + offset, generation->source, 200);
  offset += 200;
}

iw_preview_p 
iw_preview_decode(const unsigned char* bytes, 
                  size_t buf_len)
{
  iw_preview_p ret = iw_preview_init();
  size_t offset = 0;
  size_t block_bytes = 0;

  // length
  memcpy((void*)&ret->length, bytes + offset, 4);
  offset += 4;
  // type
  memcpy((void*)ret->type, bytes + offset, 2);
  offset += 2;
  // file_type
  memcpy((void*)ret->file_type, bytes + offset, 2);
  offset += 2;
  // source
  memcpy((void*)ret->source, bytes + offset, 200);
  offset += 200;

  return ret;
}

void
iw_preview_encode(const iw_preview_p preview, 
                  unsigned char** bytes,
                  size_t* size)
{
  size_t offset = 0;
  size_t total_bytes = 0;
  size_t block_bytes = 0;
  // length    
  total_bytes += 4; 
  // type    
  total_bytes += 2; 
  // file_type    
  total_bytes += 2; 
  // source    
  total_bytes += 200; 
  *size = total_bytes;
  *bytes = (unsigned char*)malloc(total_bytes);
  // length
  memcpy((*bytes) + offset, &preview->length, 4);
  offset += 4;
  // type
  memcpy((*bytes) + offset, preview->type, 2);
  offset += 2;
  // file_type
  memcpy((*bytes) + offset, preview->file_type, 2);
  offset += 2;
  // source
  memcpy((*bytes) + offset, preview->source, 200);
  offset += 200;
}
