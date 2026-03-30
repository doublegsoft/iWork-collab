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

static struct lws_context* context = NULL;

/*!
** WebSocket request processing callback
**
** Handles lifecycle and message events from libwebsockets.
**
** @param wsi    WebSocket connection instance
** @param reason Callback reason (event type)
** @param user   Per-session user data (unused here)
** @param in     Input data buffer
** @param len    Length of input data
**
** @return int   0 on success
*/
static int 
iw_request_process(struct lws* wsi, enum lws_callback_reasons reason,
                   void* user, void* in, size_t len) {
  switch (reason) 
  {
    case LWS_CALLBACK_ESTABLISHED:
      printf("[WS] Connection established\n");
      break;

    case LWS_CALLBACK_RECEIVE: {
      printf("[WS] Received data: %.*s\n", (int)len, (char*)in);
      
      // Libwebsockets requires data to be padded with LWS_PRE bytes at the front
      unsigned char buf[LWS_PRE + 128];
      unsigned char* p = &buf[LWS_PRE];
      
      // Format a simple reply
      int n = snprintf((char*)p, 128, "Server received: %.*s", (int)len, (char*)in);
      
      // Write data back to the client
      lws_write(wsi, p, n, LWS_WRITE_TEXT);
      break;
    }

    case LWS_CALLBACK_CLOSED:
      printf("[WS] Connection closed\n");
      break;

    default:
      break;
  }
  return 0;
}

/*!
** Signal handler for graceful server shutdown
**
** This callback is triggered when a registered OS signal
** (e.g., SIGINT / Ctrl+C) is received.
**
** @param handle  libuv signal handle
** @param signum  Signal number received
**
** Behavior:
** 1. Prints shutdown message
** 2. Stops the libuv event loop
*/
static void 
iw_server_single(uv_signal_t* handle, int signum) {
  printf("\nInterrupt received. Stopping...\n");
  uv_stop(handle->loop);
}

static struct lws_protocols protocols[] = {
  {
    "iWork-protocol", // Protocol name (used by clients connecting)
    iw_request_process,   // Callback function
    0,                  // Per-session data size
    0,                  // Maximum frame size
  },
  { NULL, NULL, 0, 0 }  /* terminator */
};

int main(void) 
{
  struct lws_context_creation_info info;
  uv_loop_t* loop = uv_default_loop();
  
  // Setup graceful exit on CTRL+C
  uv_signal_t sigint;
  uv_signal_init(loop, &sigint);
  uv_signal_start(&sigint, iw_server_single, SIGINT);

  memset(&info, 0, sizeof info);
  info.port = 8848;
  info.protocols = protocols;
  
  info.options = LWS_SERVER_OPTION_LIBUV; 
  
  void* foreign_loops[1];
  foreign_loops[0] = loop;
  info.foreign_loops = foreign_loops;
  info.iface = "0.0.0.0";

  context = lws_create_context(&info);
  if (!context) {
    fprintf(stderr, "libwebsockets init failed\n");
    return -1;
  }

  printf("WebSocket Server started on ws://localhost:%d\n", info.port);
  printf("Press CTRL+C to stop.\n");

  uv_run(loop, UV_RUN_DEFAULT);

  lws_context_destroy(context);
  uv_loop_close(loop);

  return 0;
}