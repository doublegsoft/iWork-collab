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
#include <limits.h>

#include "iWork-pkt.h"

iw_prompt_p
iw_prompt_init(void)
{
  iw_prompt_p ret = (iw_prompt_p) malloc(sizeof(iw_prompt_t));
  ret->magic = 287454020;
  ret->version[0] = '\0';
  ret->request = INT_MIN;
  ret->type[0] = '\0';
  ret->length = INT_MIN;
  ret->text_length = INT_MIN;
  ret->text = NULL;
  ret->file_count = INT_MIN;
  ret->file_lens = NULL;
  ret->files = NULL;
  return ret;
}

void
iw_prompt_free(iw_prompt_p prompt)
{
  if (prompt->text != NULL) 
    free(prompt->text);
  if (prompt->file_lens != NULL)
    free(prompt->file_lens);
  if (prompt->files != NULL)
    free(prompt->files);
  free(prompt);
}

void
iw_prompt_set_magic(iw_prompt_p prompt, int value)
{
  prompt->magic = value;
}

void
iw_prompt_set_version(iw_prompt_p prompt, const char* value, size_t len)
{
  memcpy(prompt->version, value, len);
}    
    
void
iw_prompt_set_request(iw_prompt_p prompt, long value)
{
  prompt->request = value;
}

void
iw_prompt_set_type(iw_prompt_p prompt, const char* value, size_t len)
{
  memcpy(prompt->type, value, len);
}    
    
void
iw_prompt_set_length(iw_prompt_p prompt, int value)
{
  prompt->length = value;
}

void
iw_prompt_set_text_length(iw_prompt_p prompt, int value)
{
  prompt->text_length = value;
}

void
iw_prompt_set_text(iw_prompt_p prompt, const char* value, size_t len)
{
  prompt->text = (char*)malloc(sizeof(char) * (len));
  memcpy(prompt->text, value, len);
}     

void
iw_prompt_set_file_count(iw_prompt_p prompt, int value)
{
  prompt->file_count = value;
}

void
iw_prompt_set_file_lens(iw_prompt_p prompt, const int* value, size_t len)
{
  prompt->file_lens = (int*)malloc(sizeof(char) * (len));
  memcpy(prompt->file_lens, value, len);
}     

void
iw_prompt_set_files(iw_prompt_p prompt, const char* value, size_t len)
{
  prompt->files = (char*)malloc(sizeof(char) * (len));
  memcpy(prompt->files, value, len);
}     

iw_compilation_p
iw_compilation_init(void)
{
  iw_compilation_p ret = (iw_compilation_p) malloc(sizeof(iw_compilation_t));
  ret->magic = 287454020;
  ret->type[0] = '\0';
  ret->language[0] = '\0';
  ret->src_len = INT_MIN;
  ret->source = NULL;
  return ret;
}

void
iw_compilation_free(iw_compilation_p compilation)
{
  if (compilation->source != NULL) 
    free(compilation->source);
  free(compilation);
}

void
iw_compilation_set_magic(iw_compilation_p compilation, int value)
{
  compilation->magic = value;
}

void
iw_compilation_set_type(iw_compilation_p compilation, const char* value, size_t len)
{
  memcpy(compilation->type, value, len);
}    
    
void
iw_compilation_set_language(iw_compilation_p compilation, const char* value, size_t len)
{
  memcpy(compilation->language, value, len);
}    
    
void
iw_compilation_set_src_len(iw_compilation_p compilation, int value)
{
  compilation->src_len = value;
}

void
iw_compilation_set_source(iw_compilation_p compilation, const char* value, size_t len)
{
  compilation->source = (char*)malloc(sizeof(char) * (len));
  memcpy(compilation->source, value, len);
}     

iw_build_p
iw_build_init(void)
{
  iw_build_p ret = (iw_build_p) malloc(sizeof(iw_build_t));
  ret->magic = 287454020;
  ret->type[0] = '\0';
  ret->build_tool[0] = '\0';
  ret->project_path[0] = '\0';
  return ret;
}

void
iw_build_free(iw_build_p build)
{
  free(build);
}

void
iw_build_set_magic(iw_build_p build, int value)
{
  build->magic = value;
}

void
iw_build_set_type(iw_build_p build, const char* value, size_t len)
{
  memcpy(build->type, value, len);
}    
    
void
iw_build_set_build_tool(iw_build_p build, const char* value, size_t len)
{
  memcpy(build->build_tool, value, len);
}    
    
void
iw_build_set_project_path(iw_build_p build, const char* value, size_t len)
{
  memcpy(build->project_path, value, len);
}    
    
iw_generation_p
iw_generation_init(void)
{
  iw_generation_p ret = (iw_generation_p) malloc(sizeof(iw_generation_t));
  ret->magic = 287454020;
  ret->type[0] = '\0';
  ret->file_type[0] = '\0';
  ret->source[0] = '\0';
  return ret;
}

void
iw_generation_free(iw_generation_p generation)
{
  free(generation);
}

void
iw_generation_set_magic(iw_generation_p generation, int value)
{
  generation->magic = value;
}

void
iw_generation_set_type(iw_generation_p generation, const char* value, size_t len)
{
  memcpy(generation->type, value, len);
}    
    
void
iw_generation_set_file_type(iw_generation_p generation, const char* value, size_t len)
{
  memcpy(generation->file_type, value, len);
}    
    
void
iw_generation_set_source(iw_generation_p generation, const char* value, size_t len)
{
  memcpy(generation->source, value, len);
}    
    
iw_preview_p
iw_preview_init(void)
{
  iw_preview_p ret = (iw_preview_p) malloc(sizeof(iw_preview_t));
  ret->magic = 287454020;
  ret->type[0] = '\0';
  ret->file_type[0] = '\0';
  ret->source[0] = '\0';
  return ret;
}

void
iw_preview_free(iw_preview_p preview)
{
  free(preview);
}

void
iw_preview_set_magic(iw_preview_p preview, int value)
{
  preview->magic = value;
}

void
iw_preview_set_type(iw_preview_p preview, const char* value, size_t len)
{
  memcpy(preview->type, value, len);
}    
    
void
iw_preview_set_file_type(iw_preview_p preview, const char* value, size_t len)
{
  memcpy(preview->file_type, value, len);
}    
    
void
iw_preview_set_source(iw_preview_p preview, const char* value, size_t len)
{
  memcpy(preview->source, value, len);
}    
    
