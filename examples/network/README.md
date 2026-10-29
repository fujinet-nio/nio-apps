# network - sessions

One set of calls (`fn_open`, `fn_info`, `fn_read`, `fn_write`, `fn_close`) serves HTTP, HTTPS, TCP and TLS; the
URL scheme chooses. Everything is asynchronous: **`FN_ERR_NOT_READY` means "not yet", not "failed".**

Read them in this order:

| Program | Shows |
|---|---|
| `httpget/` | The whole lifecycle for a request with no body; NOT_READY; offsets; EOF. |
| `httppost/` | A request body: `FN_OPEN_BODY_UNKNOWN`, `fn_write`, the zero-length commit. |
| `httphead/` | GET and POST with `fn_open_headers`, a known body length, and reads larger than one packet. |
| `jsonq/` | Server-side JSON extraction with `fn_open_translated`. |
| `tcppipe/` | A permanent connection as a pipe: `FN_OPEN_KEEP`, framing, `fn_read_available`. |
| `waitany/` | Several sessions at once: `fn_ready`, and the hardware hint to wait without polling. |
| `httpbin/` | `fhttpbin`: a table-driven check of the HTTP client against httpbin, for regression testing. |

Test servers: `fujinet-nio/scripts/start_test_services.sh http` (httpbin on 8080) and `... tcp` (echo on 7777).
The 8-bit machines have no command line: build the address in with
`make TARGET=atari HTTPBIN_URL=http://<pc>:8080 TCP_URL=tcp://<pc>:7777`.

For example, `httphead http://<pc>:8080` sends a GET to `/headers` followed by
a POST to `/post`, both with a demonstration Authorization header. The POST
declares its JSON body length at open and writes those bytes; it does not need
the zero-length commit used for unknown-length bodies in `httppost`.

On Amiga, Linux and MS-DOS, `httphead` passes a 1024-byte application buffer to
`fn_read`. The broker still limits each packet to 1024 bytes including framing:
the library may perform multiple exchanges to fill the buffer. A temporary
not-ready response after some data causes a successful short read, so always
advance by the returned byte count. On Atari this example uses a 128-byte
buffer and each call returns at most one packet. BBC's ROM-backed network open
does not support arbitrary request headers, so the example is excluded there.

Run the isolated example behavior test with `make test-httphead`.
