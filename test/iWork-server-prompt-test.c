/*
**    ▄▄▄▄  ▄▄▄  ▄▄▄▄
** ▀▀ ▀███  ███  ███▀            ▄▄
** ██  ███  ███  ███ ▄███▄ ████▄ ██ ▄█▀
** ██  ███▄▄███▄▄███ ██ ██ ██ ▀▀ ████
** ██▄  ▀████▀████▀  ▀███▀ ██    ██ ▀█▄
*/

/*!
** iWork-server-prompt-test
**
** 测试 iWork-server 的 prompt 功能。
**
** 1. 本地 round-trip 测试
** 2. 连接到 iWork-server 并发送 prompt
*/
#include <libwebsockets.h>

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iWork-pkt-codec.h"
#include "iWork-pkt.h"
#include "../src/iWork-ws.h"

/*! 使用 gen 侧 iw_prompt 指针编码为 wire（对应 iw_prompt_encode 的封装）。 */
static int
iw_prompt_packet_encode(const iw_prompt_p prompt,
                        unsigned char** bytes,
                        size_t* size);

static iw_prompt_p
iw_prompt_packet_decode(const unsigned char* wire, size_t wire_len);

/*! 构造测试用 iw_prompt（gen/iWork-pkt.h 的 setters）。 */
static iw_prompt_p
iw_test_prompt_new(const char* text);

/* --- 发到服务器的测试状态 --- */
static struct lws_context* g_context = NULL;
static volatile int g_done = 0;
static volatile int g_sent = 0;
static volatile int g_ack_verified = 0;
static const char g_server_test_text[] = "hello, world!";

static void
iw_client_stop(void)
{
  g_done = 1;
  if (g_context != NULL) {
    lws_cancel_service(g_context);
  }
}

static int
iw_prompt_ws_callback(struct lws* wsi,
                      enum lws_callback_reasons reason,
                      void* user,
                      void* in,
                      size_t len)
{
  (void)user;

  switch (reason) {
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
      printf("[prompt-test] connected\n");
      lws_callback_on_writable(wsi);
      break;

    case LWS_CALLBACK_CLIENT_WRITEABLE: {
      unsigned char* encoded = NULL;
      size_t encoded_size = 0;
      unsigned char* buf = NULL;

      if (g_sent) {
        break;
      }

      {
        iw_prompt_p prompt = iw_test_prompt_new(g_server_test_text);
        if (prompt == NULL) {
          fprintf(stderr, "[prompt-test] iw_test_prompt_new failed\n");
          return -1;
        }
        if (iw_prompt_packet_encode(prompt, &encoded, &encoded_size) != 0) {
          iw_prompt_free(prompt);
          fprintf(stderr, "[prompt-test] encode failed\n");
          return -1;
        }
        iw_prompt_free(prompt);
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
        fprintf(stderr, "[prompt-test] send failed\n");
        return -1;
      }
      free(buf);

      g_sent = 1;
      printf("[prompt-test] sent prompt packet (%zu bytes)\n", encoded_size);
      break;
    }

    case LWS_CALLBACK_CLIENT_RECEIVE: {
      iw_prompt_p resp = iw_prompt_packet_decode((const unsigned char*)in, len);
      const char* expect_ack = "ACK:ping";

      if (resp == NULL) {
        fprintf(stderr, "[prompt-test] invalid response packet\n");
        iw_client_stop();
        break;
      }

      printf("[prompt-test] response request=%ld text=%.*s\n",
             resp->request,
             resp->text_length,
             resp->text != NULL ? resp->text : "");

      if (resp->text != NULL &&
          resp->text_length == (int)strlen(expect_ack) &&
          memcmp(resp->text, expect_ack, (size_t)resp->text_length) == 0) {
        g_ack_verified = 1;
        printf("[prompt-test] server ACK verified\n");
      } else {
        fprintf(stderr, "[prompt-test] unexpected ACK payload\n");
      }

      iw_prompt_free(resp);
      iw_client_stop();
      break;
    }

    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
      fprintf(stderr, "[prompt-test] connection error: %s\n",
              in != NULL ? (const char*)in : "(null)");
      iw_client_stop();
      break;

    case LWS_CALLBACK_CLIENT_CLOSED:
      printf("[prompt-test] disconnected\n");
      iw_client_stop();
      break;

    default:
      break;
  }

  return 0;
}

static struct lws_protocols protocols[] = {
  { "iw_prompt_packet_test", iw_prompt_ws_callback, 0, 1024, 0, NULL, 0 },
  LWS_PROTOCOL_LIST_TERM
};

static void
signal_cb(int signum)
{
  (void)signum;
  iw_client_stop();
}

static int
iw_prompt_packet_encode(const iw_prompt_p prompt,
                        unsigned char** bytes,
                        size_t* size)
{
  if (prompt == NULL || bytes == NULL || size == NULL) {
    return -1;
  }

  *bytes = NULL;
  *size = 0;

  iw_prompt_encode(prompt, bytes, size);

  if (*bytes == NULL || *size == 0) {
    return -1;
  }
  return 0;
}

static iw_prompt_p
iw_test_prompt_new(const char* text)
{
  iw_prompt_p prompt;
  const char* payload = text != NULL ? text : "";
  int text_len = (int)strlen(payload);

  prompt = iw_prompt_init();
  if (prompt == NULL) {
    return NULL;
  }

  iw_prompt_set_magic(prompt, 1769947755);
  iw_prompt_set_version(prompt, "01", 2);
  iw_prompt_set_request(prompt, 1L);
  iw_prompt_set_type(prompt, "PT", 2);
  iw_prompt_set_file_count(prompt, 0);
  iw_prompt_set_text_length(prompt, text_len);
  iw_prompt_set_length(prompt, text_len);
  if (text_len > 0) {
    iw_prompt_set_text(prompt, payload, (size_t)text_len);
  }

  return prompt;
}

static iw_prompt_p
iw_prompt_packet_decode(const unsigned char* wire, size_t wire_len)
{
  size_t buf_len = wire_len > sizeof(iw_prompt_t) ? wire_len : sizeof(iw_prompt_t);
  unsigned char* buf = (unsigned char*)calloc(1, buf_len);
  iw_prompt_p decoded = NULL;

  if (buf == NULL) {
    return NULL;
  }
  memcpy(buf, wire, wire_len);
  decoded = iw_prompt_decode(buf, buf_len);
  free(buf);
  return decoded;
}

static void
run_local_round_trip(void)
{
  const char* text = "ping";
  iw_prompt_p prompt = iw_test_prompt_new(text);
  unsigned char* encoded = NULL;
  size_t encoded_size = 0;
  iw_prompt_p decoded = NULL;

  printf("Running: local iw_prompt encode/decode round-trip\n");

  assert(prompt != NULL);
  assert(iw_prompt_packet_encode(prompt, &encoded, &encoded_size) == 0);
  iw_prompt_free(prompt);
  assert(encoded != NULL);
  printf("  encoded prompt packet: %zu bytes\n", encoded_size);

  decoded = iw_prompt_packet_decode(encoded, encoded_size);
  assert(decoded != NULL);
  assert(decoded->magic == 1769947755);
  assert(memcmp(decoded->version, "01", 2) == 0);
  assert(decoded->request == 1);
  assert(memcmp(decoded->type, "PT", 2) == 0);
  assert(decoded->file_count == 0);
  assert(decoded->text_length == (int)strlen(text));
  assert(decoded->text != NULL);
  assert(memcmp(decoded->text, text, (size_t)decoded->text_length) == 0);

  iw_prompt_free(decoded);
  free(encoded);

  printf("  local round-trip OK\n");
}

static int
run_server_prompt_test(void)
{
  struct lws_context_creation_info info;
  struct lws_client_connect_info client_info;

  g_sent = 0;
  g_ack_verified = 0;
  g_done = 0;

  memset(&info, 0, sizeof(info));
  memset(&client_info, 0, sizeof(client_info));
  signal(SIGINT, signal_cb);

  info.port = CONTEXT_PORT_NO_LISTEN;
  info.protocols = protocols;

  g_context = lws_create_context(&info);
  if (g_context == NULL) {
    fprintf(stderr, "[prompt-test] libwebsockets init failed\n");
    return 1;
  }

  client_info.context = g_context;
  client_info.address = iw_ws_host;
  client_info.port = iw_ws_port;
  client_info.path = iw_ws_path;
  client_info.host = client_info.address;
  client_info.origin = client_info.address;
  client_info.protocol = iw_ws_protocol;
  client_info.local_protocol_name = "iw_prompt_packet_test";
  client_info.ssl_connection = 0;

  if (lws_client_connect_via_info(&client_info) == NULL) {
    fprintf(stderr, "[prompt-test] connect failed (is iWork-server running?)\n");
    lws_context_destroy(g_context);
    g_context = NULL;
    return 1;
  }

  printf("[prompt-test] connecting ws://%s:%d%s (protocol %s)\n",
         iw_ws_host, iw_ws_port, iw_ws_path, iw_ws_protocol);
  while (!g_done) {
    if (lws_service(g_context, 0) < 0) {
      fprintf(stderr, "[prompt-test] service loop failed\n");
      break;
    }
  }

  lws_context_destroy(g_context);
  g_context = NULL;

  if (!g_ack_verified) {
    fprintf(stderr, "[prompt-test] FAILED: no valid ACK from server\n");
    return 1;
  }
  return 0;
}

int
main(int argc, char** argv)
{
  int local_only = argc > 1 && strcmp(argv[1], "--local-only") == 0;

  printf("iWork-server-prompt-test\n");

  run_local_round_trip();

  if (local_only) {
    printf("Test passed (--local-only, skip server)\n");
    return 0;
  }

  printf("Running: send prompt to iWork-server\n");
  if (run_server_prompt_test() != 0) {
    return 1;
  }

  printf("Test passed (local + server)\n");
  return 0;
}
