/**
 * @file fhttpbin.c
 * @brief A table-driven check of the HTTP client against an httpbin server.
 *
 * Where httpget and httppost show one request each in the plainest form, this program runs a list of
 * them (GET, POST, PUT, DELETE, HEAD, a 204, a redirect, an echo) and checks each answer: the HTTP
 * status, and that a known piece of text is in the body. It is a regression test for the network path
 * on a real machine: if something in the stack changes, one of these lines fails.
 *
 * What to look for in the code:
 *
 *   - Each case is a row in `cases[]`: method, path, optional request body, expected status, expected
 *     text, and the open flags it needs (FN_OPEN_BODY_UNKNOWN for a body you will commit with a
 *     zero-length write; FN_OPEN_FOLLOW_REDIR for a redirect).
 *   - run_case() is the whole session lifecycle in one place: open, write the body if any, commit it,
 *     fn_info(), read the body, close.
 *   - The read loop keeps going on FN_ERR_NOT_READY / FN_ERR_BUSY up to a retry limit, and stops on
 *     FN_READ_EOF or a zero-byte read. This is the "not ready is not an error" rule from httpget.
 *
 * Run: fhttpbin [base-url]     (default: the httpbin address in nio_example.h, or the FN_HTTPBIN_URL
 *                                environment variable where the machine has one; start the server with
 *                                fujinet-nio/scripts/start_test_services.sh http. To change the compiled-in
 *                                address on an 8-bit machine: make TARGET=atari HTTPBIN_URL=http://...)
 */

#include "fujinet-nio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nio_example.h"   /* FN_DEFAULT_HTTPBIN_URL, NIO_SMALL_MEMORY */

#ifdef NIO_SMALL_MEMORY
#define HTTPBIN_READ_CHUNK 192
#define HTTPBIN_URL_MAX 128
#else
#define HTTPBIN_READ_CHUNK 384
#define HTTPBIN_URL_MAX 256
#endif

#define HTTPBIN_MAX_BODY 768

typedef struct httpbin_case_s {
  const char *name;
  uint8_t method;
  const char *path;
  const char *body;
  uint16_t expect_status;
  const char *expect_body;
  uint8_t flags;
} httpbin_case_t;

static uint8_t read_buf[HTTPBIN_READ_CHUNK + 1];
static char body_buf[HTTPBIN_MAX_BODY + 1];
static char url_buf[HTTPBIN_URL_MAX + 1];
#ifdef FHTTPBIN_PROMPT_URL
static char input_buf[HTTPBIN_URL_MAX + 1];
#endif

static const httpbin_case_t cases[] = {
  { "GET", FN_METHOD_GET, "/get", NULL, 200, "\"url\"", 0 },
  { "POST", FN_METHOD_POST, "/post", "{\"test\":\"post\"}", 200, "\"json\"", FN_OPEN_BODY_UNKNOWN },
  { "PUT", FN_METHOD_PUT, "/put", "{\"test\":\"put\"}", 200, "\"json\"", FN_OPEN_BODY_UNKNOWN },
  { "DELETE", FN_METHOD_DELETE, "/delete", NULL, 200, "\"url\"", 0 },
  { "HEAD", FN_METHOD_HEAD, "/get", NULL, 200, NULL, 0 },
  { "STATUS204", FN_METHOD_GET, "/status/204", NULL, 204, NULL, 0 },
  { "REDIRECT", FN_METHOD_GET, "/redirect/1", NULL, 200, "\"url\"", FN_OPEN_FOLLOW_REDIR },
  { "ANYTHING", FN_METHOD_POST, "/anything", "{\"choices\":[1,2,3]}", 200, "\"choices\"", FN_OPEN_BODY_UNKNOWN }
};

static void usage(void)
{
  puts("FHTTPBIN [base-url]");
  puts("Default: " FN_DEFAULT_HTTPBIN_URL);
#ifndef NIO_SMALL_MEMORY
  puts("Env: FN_HTTPBIN_URL");
#endif
}

#ifdef FHTTPBIN_PROMPT_URL
static void trim_line(char *s)
{
  char *p;

  p = strchr(s, '\n');
  if (p)
    *p = 0;
  p = strchr(s, '\r');
  if (p)
    *p = 0;
}
#endif

/* Where the httpbin server is. Command line first (every machine that has one), then the environment
 * (not on the 8-bit C libraries), then, if built with FHTTPBIN_PROMPT_URL, a prompt, and finally the
 * address compiled in (see nio_example.h). */
static const char *configured_base_url(int argc, char **argv)
{
#ifndef NIO_SMALL_MEMORY
  const char *env_url;
#endif

  if (argc > 1)
    return argv[1];

#ifndef NIO_SMALL_MEMORY
  env_url = getenv("FN_HTTPBIN_URL");
  if (env_url && env_url[0])
    return env_url;
#endif

#ifdef FHTTPBIN_PROMPT_URL
  printf("HTTPBIN URL\n[%s]\n> ", FN_DEFAULT_HTTPBIN_URL);
  fflush(stdout);
  if (!fgets(input_buf, sizeof(input_buf), stdin))
    input_buf[0] = 0;
  trim_line(input_buf);
  if (input_buf[0])
    return input_buf;
#endif

  return FN_DEFAULT_HTTPBIN_URL;
}

static int build_url(const char *base_url, const char *path)
{
  size_t base_len;
  size_t path_len;
  int slash_base;
  int slash_path;

  base_len = strlen(base_url);
  path_len = strlen(path);
  slash_base = base_len > 0 && base_url[base_len - 1] == '/';
  slash_path = path_len > 0 && path[0] == '/';

  if (base_len + path_len + 2 > sizeof(url_buf))
    return 0;

  strcpy(url_buf, base_url);
  if (slash_base && slash_path) {
    strcat(url_buf, path + 1);
  } else if (!slash_base && !slash_path) {
    strcat(url_buf, "/");
    strcat(url_buf, path);
  } else {
    strcat(url_buf, path);
  }
  return 1;
}

static uint8_t get_info(fn_handle_t handle, uint16_t *http_status)
{
  uint8_t result;
  uint8_t info_flags = 0;
  uint16_t status = 0;
  uint32_t content_length = 0;

  *http_status = 0;
  result = fn_info(handle, &status, &content_length, &info_flags);
  if (result != FN_OK)
    return result;

  if (info_flags & FN_INFO_HAS_STATUS) {
    *http_status = status;
    printf(" status=%u", (unsigned) *http_status);
  }
  if (info_flags & FN_INFO_HAS_LENGTH)
    printf(" len=%lu", (unsigned long) content_length);
  return FN_OK;
}

static uint8_t read_body(fn_handle_t handle, uint32_t *total_out)
{
  uint8_t result;
  uint8_t flags = 0;
  uint16_t bytes_read = 0;
  uint32_t offset = 0;
  uint16_t stored = 0;
  uint16_t copy_len;
  uint16_t retries = 500;

  body_buf[0] = 0;

  for (;;) {
    result = fn_read(handle, offset, read_buf, HTTPBIN_READ_CHUNK, &bytes_read, &flags);
    if (result == FN_ERR_NOT_READY || result == FN_ERR_BUSY) {
      if (retries-- > 0)
        continue;
      return result;
    }
    if (result != FN_OK)
      return result;

    retries = 500;
    if (bytes_read == 0)
      break;

    copy_len = bytes_read;
    if (stored + copy_len > HTTPBIN_MAX_BODY)
      copy_len = (uint16_t)(HTTPBIN_MAX_BODY - stored);
    if (copy_len) {
      memcpy(body_buf + stored, read_buf, copy_len);
      stored = (uint16_t)(stored + copy_len);
      body_buf[stored] = 0;
    }

    offset += bytes_read;
    if (flags & FN_READ_EOF)
      break;
  }

  if (total_out)
    *total_out = offset;
  return FN_OK;
}

static int run_case(const char *base_url, const httpbin_case_t *test)
{
  uint8_t result;
  fn_handle_t handle = FN_INVALID_HANDLE;
  uint16_t written = 0;
  uint16_t http_status = 0;
  uint32_t total = 0;
  int ok = 1;

  printf("%s ", test->name);
  if (!build_url(base_url, test->path)) {
    puts("URL too long");
    return 0;
  }

  result = fn_open(&handle, test->method, url_buf, test->flags);
  if (result != FN_OK) {
    printf("open=%u(%s)\n", (unsigned) result, fn_error_string(result));
    return 0;
  }

  if (test->body) {
    result = fn_write(handle, 0, (const uint8_t *) test->body, (uint16_t) strlen(test->body), &written);
    if (result != FN_OK) {
      printf("write=%u(%s)\n", (unsigned) result, fn_error_string(result));
      fn_close(handle);
      return 0;
    }
    printf("w=%u", (unsigned) written);

    result = fn_write(handle, written, NULL, 0, &written);
    if (result != FN_OK) {
      printf(" commit=%u(%s)\n", (unsigned) result, fn_error_string(result));
      fn_close(handle);
      return 0;
    }
  }

  result = get_info(handle, &http_status);
  if (result == FN_OK && http_status && http_status != test->expect_status) {
    printf(" expected=%u", (unsigned) test->expect_status);
    ok = 0;
  }

  if (test->method != FN_METHOD_HEAD) {
    result = read_body(handle, &total);
    if (result != FN_OK) {
      printf(" read=%u(%s)\n", (unsigned) result, fn_error_string(result));
      fn_close(handle);
      return 0;
    }
    printf(" bytes=%lu", (unsigned long) total);
  }

  if (test->expect_body && !strstr(body_buf, test->expect_body)) {
    printf(" missing=%s", test->expect_body);
    ok = 0;
  }

  result = fn_close(handle);
  if (result != FN_OK) {
    printf(" close=%u(%s)", (unsigned) result, fn_error_string(result));
    ok = 0;
  }

  puts(ok ? " OK" : " FAIL");
  return ok;
}

int main(int argc, char **argv)
{
  const char *base_url;
  unsigned i;
  unsigned pass = 0;
  unsigned fail = 0;
  uint8_t result;

  if (argc > 2 || (argc > 1 && argv[1][0] == '?')) {
    usage();
    return 1;
  }

  base_url = configured_base_url(argc, argv);
  printf("FHTTPBIN\n%s\n", base_url);

  result = fn_init();
  if (result != FN_OK) {
    printf("init=%u(%s)\n", (unsigned) result, fn_error_string(result));
    return 2;
  }

  if (!fn_is_ready()) {
    puts("FujiNet not ready");
    return 2;
  }

  for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
    if (run_case(base_url, &cases[i]))
      pass++;
    else
      fail++;
  }

  printf("PASS=%u FAIL=%u\n", pass, fail);
  return fail ? 1 : 0;
}
