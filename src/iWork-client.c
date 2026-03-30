#include <libwebsockets.h>

#include <signal.h>
#include <stdio.h>
#include <string.h>

#include "iWork-ws.h"

static struct lws_context* g_context = NULL;
static int g_done = 0;

static void
iw_client_stop(void)
{
  g_done = 1;
  if (g_context != NULL) {
    lws_cancel_service(g_context);
  }
}

static int
iw_ws_callback_client(struct lws* wsi,
                      enum lws_callback_reasons reason,
                      void* user,
                      void* in,
                      size_t len)
{
  (void)user;

  switch (reason) {
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
      printf("[client] connected\n");
      lws_callback_on_writable(wsi);
      break;

    case LWS_CALLBACK_CLIENT_WRITEABLE: {
      iw_ws_message_t request;
      char message[768];
      unsigned char buffer[LWS_PRE + sizeof(message)];
      unsigned char* payload = &buffer[LWS_PRE];
      int written = 0;

      memset(&request, 0, sizeof(request));
      snprintf(request.prompt, sizeof(request.prompt),
               "%s", "generate websocket server");
      snprintf(request.instruction, sizeof(request.instruction),
               "%s", "use libwebsockets with cmake");
      snprintf(request.role, sizeof(request.role), "%s", "user");

      written = iw_ws_build_message(&request, message, sizeof(message));
      if (written < 0) {
        fprintf(stderr, "[client] failed to build message\n");
        return -1;
      }

      memcpy(payload, message, (size_t)written);
      if (lws_write(wsi, payload, (size_t)written, LWS_WRITE_TEXT) < written) {
        fprintf(stderr, "[client] failed to send message\n");
        return -1;
      }

      printf("[client] request sent\n");
      break;
    }

    case LWS_CALLBACK_CLIENT_RECEIVE: {
      iw_ws_message_t response;

      memset(&response, 0, sizeof(response));
      if (iw_ws_parse_message((const char*)in, len, &response) == 0) {
        printf("[client] prompt=%s\n", response.prompt);
        printf("[client] instruction=%s\n", response.instruction);
        printf("[client] role=%s\n", response.role);
      } else {
        printf("[client] raw response: %.*s\n", (int)len, (const char*)in);
      }

      iw_client_stop();
      break;
    }

    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
      fprintf(stderr, "[client] connection error: %s\n",
              in != NULL ? (const char*)in : "(null)");
      iw_client_stop();
      break;

    case LWS_CALLBACK_CLIENT_CLOSED:
      printf("[client] disconnected\n");
      iw_client_stop();
      break;

    default:
      break;
  }

  return 0;
}

static struct lws_protocols protocols[] = {
  { "iw_client", iw_ws_callback_client, 0, 1024, 0, NULL, 0 },
  LWS_PROTOCOL_LIST_TERM
};

static void
signal_cb(int signum)
{
  (void)signum;
  iw_client_stop();
}

int
main(void)
{
  struct lws_context_creation_info info;
  struct lws_client_connect_info client_info;

  memset(&info, 0, sizeof(info));
  memset(&client_info, 0, sizeof(client_info));
  signal(SIGINT, signal_cb);

  info.port = CONTEXT_PORT_NO_LISTEN;
  info.protocols = protocols;

  g_context = lws_create_context(&info);
  if (g_context == NULL) {
    fprintf(stderr, "[client] libwebsockets init failed\n");
    return 1;
  }

  client_info.context = g_context;
  client_info.address = iw_ws_host;
  client_info.port = iw_ws_port;
  client_info.path = iw_ws_path;
  client_info.host = client_info.address;
  client_info.origin = client_info.address;
  client_info.protocol = iw_ws_protocol;
  client_info.local_protocol_name = "iw_client";
  client_info.ssl_connection = 0;

  if (lws_client_connect_via_info(&client_info) == NULL) {
    fprintf(stderr, "[client] failed to connect to server\n");
    lws_context_destroy(g_context);
    return 1;
  }

  printf("[client] connecting to ws://%s:%d%s\n",
         iw_ws_host,
         iw_ws_port, iw_ws_path);
  while (!g_done) {
    if (lws_service(g_context, 0) < 0) {
      fprintf(stderr, "[client] service loop failed\n");
      break;
    }
  }

  lws_context_destroy(g_context);
  return 0;
}
