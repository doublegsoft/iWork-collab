#include <assert.h>
#include <libwebsockets.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iw-pkt-codec.h"
#include "iw-pkt.h"
#include "../src/iw-ws.h"

static iw_coding_p
iw_test_coding_new(const char* text);

static int
run_server_coding_test(void);

static void
iw_client_stop(void);

static int
iw_coding_ws_callback(struct lws* wsi,
                      enum lws_callback_reasons reason,
                      void* user,
                      void* in,
                      size_t len);

static void
signal_cb(int signum);

static struct lws_context* g_context = NULL;
static volatile int g_done = 0;
static volatile int g_sent = 0;
static const char g_server_test_text[] = "generate packet";

static iw_coding_p
iw_test_coding_new(const char* text)
{
  iw_coding_p coding;
  const char* payload = text != NULL ? text : "";
  int text_len = (int)strlen(payload);

  coding = iw_coding_init();
  if (coding == NULL) {
    return NULL;
  }

  iw_coding_set_magic(coding, 287454020);
  iw_coding_set_type(coding, "CD", 2);
  iw_coding_set_version(coding, "01", 2);
  iw_coding_set_request(coding, 7L);
  iw_coding_set_text_length(coding, text_len);
  if (text_len > 0) {
    iw_coding_set_text(coding, payload, (size_t)text_len);
  }

  return coding;
}

static void
iw_client_stop(void)
{
  g_done = 1;
  if (g_context != NULL) {
    lws_cancel_service(g_context);
  }
}

static int
iw_coding_ws_callback(struct lws* wsi,
                      enum lws_callback_reasons reason,
                      void* user,
                      void* in,
                      size_t len)
{
  (void)user;

  switch (reason) {
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
      printf("[generation-test] connected\n");
      lws_callback_on_writable(wsi);
      break;

    case LWS_CALLBACK_CLIENT_WRITEABLE: {
      iw_coding_p coding = NULL;
      unsigned char* encoded = NULL;
      size_t encoded_size = 0;
      unsigned char* buf = NULL;

      if (g_sent) {
        break;
      }

      coding = iw_test_coding_new(g_server_test_text);
      if (coding == NULL) {
        fprintf(stderr, "[coding-test] iw_test_coding_new failed\n");
        return -1;
      }
      iw_coding_encode(coding, &encoded, &encoded_size);
      if (encoded == NULL || encoded_size == 0) {
        iw_coding_free(coding);
        fprintf(stderr, "[coding-test] encode failed\n");
        return -1;
      }
      iw_coding_free(coding);

      buf = (unsigned char*)malloc(LWS_PRE + encoded_size);
      if (buf == NULL) {
        free(encoded);
        return -1;
      }

      memcpy(buf + LWS_PRE, encoded, encoded_size);
      free(encoded);

      if (lws_write(wsi, buf + LWS_PRE, encoded_size, LWS_WRITE_BINARY) < (int)encoded_size) {
        free(buf);
        fprintf(stderr, "[coding-test] send failed\n");
        iw_client_stop();
        return -1;
      }
      free(buf);

      g_sent = 1;
      printf("[coding-test] sent coding packet (%zu bytes)\n", encoded_size);
      // iw_client_stop();
      break;
    }

    case LWS_CALLBACK_CLIENT_RECEIVE: {
      printf("[coding-test] received response (%zu bytes)\n", len);
      iw_client_stop();
      break;
    }

    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
      fprintf(stderr, "[coding-test] connection error: %s\n",
              in != NULL ? (const char*)in : "(null)");
      iw_client_stop();
      break;

    case LWS_CALLBACK_CLIENT_CLOSED:
      printf("[coding-test] disconnected\n");
      iw_client_stop();
      break;

    default:
      break;
  }

  return 0;
}

static struct lws_protocols protocols[] = {
  { "iw_coding_packet_test", iw_coding_ws_callback, 0, 1024, 0, NULL, 0 },
  LWS_PROTOCOL_LIST_TERM
};

static void
signal_cb(int signum)
{
  (void)signum;
  iw_client_stop();
}

static void
run_local_round_trip(void)
{
  const char* text = "generate packet";
  iw_coding_p coding = iw_test_coding_new(text);
  unsigned char* encoded = NULL;
  size_t encoded_size = 0;
  iw_coding_p decoded = NULL;

  printf("Running: local iw_coding encode/decode round-trip\n");

  assert(coding != NULL);
  iw_coding_encode(coding, &encoded, &encoded_size);
  if (encoded == NULL || encoded_size == 0) {
    iw_coding_free(coding);
    fprintf(stderr, "[coding-test] encode failed\n");
    return;
  }
  iw_coding_free(coding);

  assert(encoded != NULL);
  printf("  encoded coding packet: %zu bytes\n", encoded_size);

  decoded = iw_generation_decode(encoded, encoded_size);
  assert(decoded != NULL);
  assert(decoded->magic == 287454020);
  assert(memcmp(decoded->type, "GN", 2) == 0);
  assert(memcmp(decoded->version, "01", 2) == 0);
  assert(decoded->request == 7L);
  assert(decoded->text_length == (int)strlen(text));
  assert(decoded->text != NULL);
  assert(memcmp(decoded->text, text, (size_t)decoded->text_length) == 0);

  iw_generation_free(decoded);
  free(encoded);

  printf("  local round-trip OK\n");
}

static int
run_server_coding_test(void)
{
  struct lws_context_creation_info info;
  struct lws_client_connect_info client_info;

  g_sent = 0;
  g_done = 0;

  memset(&info, 0, sizeof(info));
  memset(&client_info, 0, sizeof(client_info));
  signal(SIGINT, signal_cb);

  info.port = CONTEXT_PORT_NO_LISTEN;
  info.protocols = protocols;

  g_context = lws_create_context(&info);
  if (g_context == NULL) {
    fprintf(stderr, "[generation-test] libwebsockets init failed\n");
    return 1;
  }

  client_info.context = g_context;
  client_info.address = "localhost";
  client_info.port = iw_ws_port;
  client_info.path = iw_ws_path;
  client_info.host = client_info.address;
  client_info.origin = client_info.address;
  client_info.protocol = iw_ws_protocol;
  client_info.local_protocol_name = "iw_generation_packet_test";
  client_info.ssl_connection = 0;

  if (lws_client_connect_via_info(&client_info) == NULL) {
    fprintf(stderr, "[generation-test] connect failed (is iw-server running?)\n");
    lws_context_destroy(g_context);
    g_context = NULL;
    return 1;
  }

  printf("[generation-test] connecting ws://localhost:%d%s (protocol %s)\n",
         iw_ws_port, iw_ws_path, iw_ws_protocol);
  while (!g_done) {
    if (lws_service(g_context, 0) < 0) {
      fprintf(stderr, "[generation-test] service loop failed\n");
      break;
    }
  }

  lws_context_destroy(g_context);
  g_context = NULL;

  if (!g_sent) {
    fprintf(stderr, "[generation-test] FAILED: packet was not sent\n");
    return 1;
  }

  return 0;
}

int
main(void)
{
  printf("iw-test-generation\n");

  printf("Running: send generation packet to ws://localhost:%d%s\n",
         iw_ws_port, iw_ws_path);
  if (run_server_coding_test() != 0) {
    return 1;
  }

  printf("Test passed (packet sent)\n");
  return 0;
}
