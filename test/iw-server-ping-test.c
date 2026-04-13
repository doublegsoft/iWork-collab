/*
**    ▄▄▄▄  ▄▄▄  ▄▄▄▄
** ▀▀ ▀███  ███  ███▀            ▄▄
** ██  ███  ███  ███ ▄███▄ ████▄ ██ ▄█▀
** ██  ███▄▄███▄▄███ ██ ██ ██ ▀▀ ████
** ██▄  ▀████▀████▀  ▀███▀ ██    ██ ▀█▄
*/
#include <libwebsockets.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iw-pkt-codec.h"
#include "../src/iw-ws.h"

static struct lws_context* g_context = NULL;
static volatile int g_done = 0;
static volatile int g_sent = 0;

static void
iw_client_stop(void)
{
  g_done = 1;
  if (g_context != NULL) {
    lws_cancel_service(g_context);
  }
}

static iw_prompt_p
iw_decode_prompt_from_frame(const unsigned char* frame, size_t len)
{
  size_t decode_len = len > sizeof(iw_prompt_t) ? len : sizeof(iw_prompt_t);
  unsigned char* decode_buffer = (unsigned char*)calloc(1, decode_len);
  iw_prompt_p prompt = NULL;

  if (decode_buffer == NULL) {
    return NULL;
  }

  memcpy(decode_buffer, frame, len);
  prompt = iw_prompt_decode(decode_buffer, decode_len);
  free(decode_buffer);
  return prompt;
}

static int
iw_ws_callback_test_client(struct lws* wsi,
                           enum lws_callback_reasons reason,
                           void* user,
                           void* in,
                           size_t len)
{
  (void)user;

  switch (reason) {
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
      printf("[test-client] connected\n");
      lws_callback_on_writable(wsi);
      break;

    case LWS_CALLBACK_CLIENT_WRITEABLE: {
      iw_prompt_p req = NULL;
      unsigned char* encoded = NULL;
      size_t encoded_size = 0;
      unsigned char* buf = NULL;

      if (g_sent) {
        break;
      }

      req = iw_prompt_init();
      if (req == NULL) {
        return -1;
      }

      req->magic = 1769947755;
      memcpy(req->version, "01", 2);
      req->request = 1;
      memcpy(req->type, "PT", 2);
      req->file_count = 0;

      {
        const char* text = "ping";
        req->text_length = (int)strlen(text);
        req->length = req->text_length;
        req->text = (char*)malloc((size_t)req->text_length);
        if (req->text == NULL) {
          iw_prompt_free(req);
          return -1;
        }
        memcpy(req->text, text, (size_t)req->text_length);
      }

      iw_prompt_encode(req, &encoded, &encoded_size);
      iw_prompt_free(req);

      if (encoded == NULL || encoded_size == 0) {
        return -1;
      }

      buf = (unsigned char*)malloc(LWS_PRE + encoded_size);
      if (buf == NULL) {
        free(encoded);
        return -1;
      }

      memcpy(buf + LWS_PRE, encoded, encoded_size);
      free(encoded);

      if (lws_write(wsi, buf + LWS_PRE, encoded_size, LWS_WRITE_BINARY) < (int)encoded_size) {
        free(buf);
        fprintf(stderr, "[test-client] send failed\n");
        return -1;
      }

      free(buf);
      g_sent = 1;
      printf("[test-client] request sent (%zu bytes)\n", encoded_size);
      break;
    }

    case LWS_CALLBACK_CLIENT_RECEIVE: {
      iw_prompt_p resp = iw_decode_prompt_from_frame((const unsigned char*)in, len);
      if (resp == NULL) {
        fprintf(stderr, "[test-client] invalid response packet\n");
        iw_client_stop();
        break;
      }

      printf("[test-client] response request=%ld text=%.*s\n",
             resp->request,
             resp->text_length,
             resp->text != NULL ? resp->text : "");

      if (resp->text == NULL || resp->text_length < 4 ||
          memcmp(resp->text, "ACK:", 4) != 0) {
        fprintf(stderr, "[test-client] response is not ACK\n");
      } else {
        printf("[test-client] ACK verified\n");
      }

      iw_prompt_free(resp);
      iw_client_stop();
      break;
    }

    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
      fprintf(stderr, "[test-client] connection error: %s\n",
              in != NULL ? (const char*)in : "(null)");
      iw_client_stop();
      break;

    case LWS_CALLBACK_CLIENT_CLOSED:
      printf("[test-client] disconnected\n");
      iw_client_stop();
      break;

    default:
      break;
  }

  return 0;
}

static struct lws_protocols protocols[] = {
  { "iw_test_client", iw_ws_callback_test_client, 0, 1024, 0, NULL, 0 },
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
    fprintf(stderr, "[test-client] libwebsockets init failed\n");
    return 1;
  }

  client_info.context = g_context;
  client_info.address = iw_ws_host;
  client_info.port = iw_ws_port;
  client_info.path = iw_ws_path;
  client_info.host = client_info.address;
  client_info.origin = client_info.address;
  client_info.protocol = iw_ws_protocol;
  client_info.local_protocol_name = "iw_test_client";
  client_info.ssl_connection = 0;

  if (lws_client_connect_via_info(&client_info) == NULL) {
    fprintf(stderr, "[test-client] failed to connect to server\n");
    lws_context_destroy(g_context);
    return 1;
  }

  printf("[test-client] connecting to ws://%s:%d%s\n", iw_ws_host, iw_ws_port, iw_ws_path);
  while (!g_done) {
    if (lws_service(g_context, 0) < 0) {
      fprintf(stderr, "[test-client] service loop failed\n");
      break;
    }
  }

  lws_context_destroy(g_context);
  return 0;
}
