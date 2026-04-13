#include "iw-ws.h"

#include <stdio.h>
#include <string.h>

static int
iw_ws_extract_field(const char* payload,
                    const char* key,
                    char* output,
                    size_t output_size)
{
  const char* start = strstr(payload, key);
  const char* end = NULL;
  size_t key_len = strlen(key);
  size_t value_len = 0;

  if (start == NULL || output == NULL || output_size == 0) {
    return -1;
  }

  start += key_len;
  end = strchr(start, '\n');
  value_len = end == NULL ? strlen(start) : (size_t)(end - start);
  if (value_len >= output_size) {
    value_len = output_size - 1;
  }

  memcpy(output, start, value_len);
  output[value_len] = '\0';
  return 0;
}

int
iw_ws_build_message(const iw_ws_message_t* message,
                    char* buffer,
                    size_t buffer_size)
{
  int written = 0;

  if (message == NULL || buffer == NULL || buffer_size == 0) {
    return -1;
  }

  written = snprintf(buffer,
                     buffer_size,
                     "prompt=%s\ninstruction=%s\nrole=%s\n",
                     message->prompt,
                     message->instruction,
                     message->role);
  if (written < 0 || (size_t)written >= buffer_size) {
    return -1;
  }

  return written;
}

int
iw_ws_parse_message(const char* payload,
                    size_t payload_len,
                    iw_ws_message_t* message)
{
  char local[1024];

  if (payload == NULL || message == NULL || payload_len >= sizeof(local)) {
    return -1;
  }

  memcpy(local, payload, payload_len);
  local[payload_len] = '\0';

  if (iw_ws_extract_field(local, "prompt=", message->prompt,
                          sizeof(message->prompt)) != 0) {
    return -1;
  }
  if (iw_ws_extract_field(local, "instruction=", message->instruction,
                          sizeof(message->instruction)) != 0) {
    return -1;
  }
  if (iw_ws_extract_field(local, "role=", message->role,
                          sizeof(message->role)) != 0) {
    return -1;
  }

  return 0;
}
