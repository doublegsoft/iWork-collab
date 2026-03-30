#ifndef iw_ws_h
#define iw_ws_h

#include <stddef.h>

#define iw_ws_port 8848
#define iw_ws_host "127.0.0.1"
#define iw_ws_protocol "iw-collab"
#define iw_ws_path "/"

typedef struct iw_ws_message_s {
  char prompt[256];
  char instruction[256];
  char role[32];
} iw_ws_message_t;

int
iw_ws_build_message(const iw_ws_message_t* message,
                    char* buffer,
                    size_t buffer_size);

int
iw_ws_parse_message(const char* payload,
                    size_t payload_len,
                    iw_ws_message_t* message);

#endif
