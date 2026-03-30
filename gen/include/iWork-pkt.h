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
#ifndef __IWORK_PKT_H__
#define __IWORK_PKT_H__

#ifdef __cplusplus
extern "C"
{
#endif

/*!
** language
*/
typedef enum 
{
  LANGUAGE_JAVA,
  LANGUAGE_C,
  LANGUAGE_CPP,
  LANGUAGE_OBJC
} 
iw_language_t;

/*!
** build_tool
*/
typedef enum 
{
  BUILD_TOOL_MAVEN,
  BUILD_TOOL_CMAKE,
  BUILD_TOOL_CARGO,
  BUILD_TOOL_YARN
} 
iw_build_tool_t;

/*!
** file_type
*/
typedef enum 
{
  FILE_TYPE_MARKDOWN,
  FILE_TYPE_CMAKE,
  FILE_TYPE_CARGO,
  FILE_TYPE_YARN
} 
iw_file_type_t;

/*!
** 【提示词】对象。
*/
typedef struct iw_prompt_s      iw_prompt_t;
typedef        iw_prompt_t*     iw_prompt_p;

/*!
** 【编译指令】对象。
*/
typedef struct iw_compilation_s      iw_compilation_t;
typedef        iw_compilation_t*     iw_compilation_p;

/*!
** 【构建指令】对象。
*/
typedef struct iw_build_s      iw_build_t;
typedef        iw_build_t*     iw_build_p;

/*!
** 【文件生成】对象。
*/
typedef struct iw_generation_s      iw_generation_t;
typedef        iw_generation_t*     iw_generation_p;

/*!
** 【预览界面】对象。
*/
typedef struct iw_preview_s      iw_preview_t;
typedef        iw_preview_t*     iw_preview_p;

/*!
** 【提示词】数据结构定义。
*/
struct iw_prompt_s 
{
  /*!
  ** 指明这个对象的类型名称。
  */
  char typename[64];

  /*!
  ** 【魔术数字】
  */
  int magic;

  /*!
  ** 【版本】
  */
  char version[2];

  /*!
  ** 【请求】
  */
  long request;

  /*!
  ** 【类型：提示词】
  */
  char type[2];

  /*!
  ** 【指令长度】
  */
  int length;

  /*!
  ** 【文本长度】
  */
  int text_length;

  /*!
  ** 【文本内容】
  */
  char* text;

  /*!
  ** 【文件个数】
  */
  int file_count;

  /*!
  ** 【各个文件长度】
  */
  int* file_lens;

  /*!
  ** 【文件内容】
  */
  char* files;
}; 

/*!
** 【编译指令】数据结构定义。
*/
struct iw_compilation_s 
{
  /*!
  ** 指明这个对象的类型名称。
  */
  char typename[64];

  /*!
  ** 【指令长度】
  */
  int len;

  /*!
  ** 【指令类型】
  */
  char type[2];

  /*!
  ** 【编程语言】
  */
  char language[2];

  /*!
  ** 【代码长度】
  */
  int src_len;

  /*!
  ** 【代码】
  */
  char* source;
}; 

/*!
** 【构建指令】数据结构定义。
*/
struct iw_build_s 
{
  /*!
  ** 指明这个对象的类型名称。
  */
  char typename[64];

  /*!
  ** 【指令长度】
  */
  int length;

  /*!
  ** 【指令类型】
  */
  char type[2];

  /*!
  ** 【编程语言】
  */
  char build_tool[2];

  /*!
  ** 【项目路径】
  */
  char project_path[200];
}; 

/*!
** 【文件生成】数据结构定义。
*/
struct iw_generation_s 
{
  /*!
  ** 指明这个对象的类型名称。
  */
  char typename[64];

  /*!
  ** 【包长度】
  */
  int length;

  /*!
  ** 【指令类型】
  */
  char type[2];

  /*!
  ** 【编程语言】
  */
  char file_type[2];

  /*!
  ** 【项目路径】
  */
  char source[200];
}; 

/*!
** 【预览界面】数据结构定义。
*/
struct iw_preview_s 
{
  /*!
  ** 指明这个对象的类型名称。
  */
  char typename[64];

  /*!
  ** 【包长度】
  */
  int length;

  /*!
  ** 【指令类型】
  */
  char type[2];

  /*!
  ** 【编程语言】
  */
  char file_type[2];

  /*!
  ** 【项目路径】
  */
  char source[200];
}; 

/*!
** 创建并初始化【提示词】对象。
*/
iw_prompt_p
iw_prompt_init(void);

/*!
** 释放【提示词】对象所占用的内存。
*/
void
iw_prompt_free(iw_prompt_p);
  
/*!
** 设置【提示词】的【魔术数字】属性值。
*/
void
iw_prompt_set_magic(iw_prompt_p, int);
  
/*!
** 设置【提示词】的【版本】属性值。
*/
void
iw_prompt_set_version(iw_prompt_p, const char*, size_t);  
  
/*!
** 设置【提示词】的【请求标识】属性值。
*/
void
iw_prompt_set_request(iw_prompt_p, long);
  
/*!
** 设置【提示词】的【类型：提示词】属性值。
*/
void
iw_prompt_set_type(iw_prompt_p, const char*, size_t);  
  
/*!
** 设置【提示词】的【指令长度】属性值。
*/
void
iw_prompt_set_length(iw_prompt_p, int);
  
/*!
** 设置【提示词】的【文本长度】属性值。
*/
void
iw_prompt_set_text_length(iw_prompt_p, int);
  
/*!
** 设置【提示词】的【文本内容】属性值。
*/
void
iw_prompt_set_text(iw_prompt_p, const char*, size_t);  
  
/*!
** 设置【提示词】的【文件个数】属性值。
*/
void
iw_prompt_set_file_count(iw_prompt_p, int);
  
/*!
** 设置【提示词】的【各个文件长度】属性值。
*/
void
iw_prompt_set_file_lens(iw_prompt_p, const int*, size_t);    

/*!
** 获取【提示词】的【各个文件长度】某个索引值。
*/
int
iw_prompt_get_file_len(iw_prompt_p, int idx);    
  
/*!
** 设置【提示词】的【文件内容】属性值。
*/
void
iw_prompt_set_files(iw_prompt_p, const char*, size_t);  

/*!
** 获取【提示词】的【文件内容】某个索引值。
*/
char*
iw_prompt_get_file(iw_prompt_p, int idx, size_t*);

/*!
** 创建并初始化【编译指令】对象。
*/
iw_compilation_p
iw_compilation_init(void);

/*!
** 释放【编译指令】对象所占用的内存。
*/
void
iw_compilation_free(iw_compilation_p);
  
/*!
** 设置【编译指令】的【指令长度】属性值。
*/
void
iw_compilation_set_len(iw_compilation_p, int);
  
/*!
** 设置【编译指令】的【指令类型】属性值。
*/
void
iw_compilation_set_type(iw_compilation_p, const char*, size_t);  
  
/*!
** 设置【编译指令】的【编程语言】属性值。
*/
void
iw_compilation_set_language(iw_compilation_p, const char*, size_t);  
  
/*!
** 设置【编译指令】的【代码长度】属性值。
*/
void
iw_compilation_set_src_len(iw_compilation_p, int);
  
/*!
** 设置【编译指令】的【代码】属性值。
*/
void
iw_compilation_set_source(iw_compilation_p, const char*, size_t);  

/*!
** 创建并初始化【构建指令】对象。
*/
iw_build_p
iw_build_init(void);

/*!
** 释放【构建指令】对象所占用的内存。
*/
void
iw_build_free(iw_build_p);
  
/*!
** 设置【构建指令】的【指令长度】属性值。
*/
void
iw_build_set_length(iw_build_p, int);
  
/*!
** 设置【构建指令】的【指令类型】属性值。
*/
void
iw_build_set_type(iw_build_p, const char*, size_t);  
  
/*!
** 设置【构建指令】的【编程语言】属性值。
*/
void
iw_build_set_build_tool(iw_build_p, const char*, size_t);  
  
/*!
** 设置【构建指令】的【项目路径】属性值。
*/
void
iw_build_set_project_path(iw_build_p, const char*, size_t);  

/*!
** 创建并初始化【文件生成】对象。
*/
iw_generation_p
iw_generation_init(void);

/*!
** 释放【文件生成】对象所占用的内存。
*/
void
iw_generation_free(iw_generation_p);
  
/*!
** 设置【文件生成】的【包长度】属性值。
*/
void
iw_generation_set_length(iw_generation_p, int);
  
/*!
** 设置【文件生成】的【指令类型】属性值。
*/
void
iw_generation_set_type(iw_generation_p, const char*, size_t);  
  
/*!
** 设置【文件生成】的【编程语言】属性值。
*/
void
iw_generation_set_file_type(iw_generation_p, const char*, size_t);  
  
/*!
** 设置【文件生成】的【项目路径】属性值。
*/
void
iw_generation_set_source(iw_generation_p, const char*, size_t);  

/*!
** 创建并初始化【预览界面】对象。
*/
iw_preview_p
iw_preview_init(void);

/*!
** 释放【预览界面】对象所占用的内存。
*/
void
iw_preview_free(iw_preview_p);
  
/*!
** 设置【预览界面】的【包长度】属性值。
*/
void
iw_preview_set_length(iw_preview_p, int);
  
/*!
** 设置【预览界面】的【指令类型】属性值。
*/
void
iw_preview_set_type(iw_preview_p, const char*, size_t);  
  
/*!
** 设置【预览界面】的【编程语言】属性值。
*/
void
iw_preview_set_file_type(iw_preview_p, const char*, size_t);  
  
/*!
** 设置【预览界面】的【项目路径】属性值。
*/
void
iw_preview_set_source(iw_preview_p, const char*, size_t);  

#ifdef __cplusplus
}
#endif

#endif // __IWORK_PKT_H__
