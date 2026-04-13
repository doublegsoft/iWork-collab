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

#ifndef __IW_PKT_CODEC_H__
#define __IW_PKT_CODEC_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include "iw-pkt.h"

/*!
** Decodes a serialized raw byte buffer into an object instance.
**
** @param bytes The raw byte array containing the serialized data.
** @return      A pointer to the newly allocated and decoded object 
**              (iw_prompt_p). The caller is typically 
**              responsible for freeing this memory.
*/
iw_prompt_p 
iw_prompt_decode(const unsigned char* buf, 
                 size_t buf_len);

/*!
** Serializes (encodes) an object instance into a dynamically allocated byte buffer.
** 
** Note: The function name here is generated as '_decode', but the parameters 
** suggest this is meant to be an '_encode' or '_serialize' function.
**
** @param obj   The object instance to be serialized.
** @param bytes A pointer to a byte array pointer. The function will allocate 
**              the required memory for the serialized data and set *bytes to 
**              point to it. The caller is responsible for freeing this buffer.
** @param size  A pointer to an unsigned int where the function will write 
**              the total size (in bytes) of the newly allocated buffer.
*/
void 
iw_prompt_encode(const iw_prompt_p obj, 
                 unsigned char** bytes, 
                 size_t* size);

/*!
** Decodes a serialized raw byte buffer into an object instance.
**
** @param bytes The raw byte array containing the serialized data.
** @return      A pointer to the newly allocated and decoded object 
**              (iw_compilation_p). The caller is typically 
**              responsible for freeing this memory.
*/
iw_compilation_p 
iw_compilation_decode(const unsigned char* buf, 
                      size_t buf_len);

/*!
** Serializes (encodes) an object instance into a dynamically allocated byte buffer.
** 
** Note: The function name here is generated as '_decode', but the parameters 
** suggest this is meant to be an '_encode' or '_serialize' function.
**
** @param obj   The object instance to be serialized.
** @param bytes A pointer to a byte array pointer. The function will allocate 
**              the required memory for the serialized data and set *bytes to 
**              point to it. The caller is responsible for freeing this buffer.
** @param size  A pointer to an unsigned int where the function will write 
**              the total size (in bytes) of the newly allocated buffer.
*/
void 
iw_compilation_encode(const iw_compilation_p obj, 
                      unsigned char** bytes, 
                      size_t* size);

/*!
** Decodes a serialized raw byte buffer into an object instance.
**
** @param bytes The raw byte array containing the serialized data.
** @return      A pointer to the newly allocated and decoded object 
**              (iw_build_p). The caller is typically 
**              responsible for freeing this memory.
*/
iw_build_p 
iw_build_decode(const unsigned char* buf, 
                size_t buf_len);

/*!
** Serializes (encodes) an object instance into a dynamically allocated byte buffer.
** 
** Note: The function name here is generated as '_decode', but the parameters 
** suggest this is meant to be an '_encode' or '_serialize' function.
**
** @param obj   The object instance to be serialized.
** @param bytes A pointer to a byte array pointer. The function will allocate 
**              the required memory for the serialized data and set *bytes to 
**              point to it. The caller is responsible for freeing this buffer.
** @param size  A pointer to an unsigned int where the function will write 
**              the total size (in bytes) of the newly allocated buffer.
*/
void 
iw_build_encode(const iw_build_p obj, 
                unsigned char** bytes, 
                size_t* size);

/*!
** Decodes a serialized raw byte buffer into an object instance.
**
** @param bytes The raw byte array containing the serialized data.
** @return      A pointer to the newly allocated and decoded object 
**              (iw_generation_p). The caller is typically 
**              responsible for freeing this memory.
*/
iw_generation_p 
iw_generation_decode(const unsigned char* buf, 
                     size_t buf_len);

/*!
** Serializes (encodes) an object instance into a dynamically allocated byte buffer.
** 
** Note: The function name here is generated as '_decode', but the parameters 
** suggest this is meant to be an '_encode' or '_serialize' function.
**
** @param obj   The object instance to be serialized.
** @param bytes A pointer to a byte array pointer. The function will allocate 
**              the required memory for the serialized data and set *bytes to 
**              point to it. The caller is responsible for freeing this buffer.
** @param size  A pointer to an unsigned int where the function will write 
**              the total size (in bytes) of the newly allocated buffer.
*/
void 
iw_generation_encode(const iw_generation_p obj, 
                     unsigned char** bytes, 
                     size_t* size);

/*!
** Decodes a serialized raw byte buffer into an object instance.
**
** @param bytes The raw byte array containing the serialized data.
** @return      A pointer to the newly allocated and decoded object 
**              (iw_preview_p). The caller is typically 
**              responsible for freeing this memory.
*/
iw_preview_p 
iw_preview_decode(const unsigned char* buf, 
                  size_t buf_len);

/*!
** Serializes (encodes) an object instance into a dynamically allocated byte buffer.
** 
** Note: The function name here is generated as '_decode', but the parameters 
** suggest this is meant to be an '_encode' or '_serialize' function.
**
** @param obj   The object instance to be serialized.
** @param bytes A pointer to a byte array pointer. The function will allocate 
**              the required memory for the serialized data and set *bytes to 
**              point to it. The caller is responsible for freeing this buffer.
** @param size  A pointer to an unsigned int where the function will write 
**              the total size (in bytes) of the newly allocated buffer.
*/
void 
iw_preview_encode(const iw_preview_p obj, 
                  unsigned char** bytes, 
                  size_t* size);

#ifdef __cplusplus
}
#endif

#endif // __IW_PKT_CODEC_H__
