# network - sessions

One set of calls (`fn_open`, `fn_info`, `fn_read`, `fn_write`, `fn_close`) serves HTTP, HTTPS, TCP and TLS; the
URL scheme chooses. Everything is asynchronous: **`FN_ERR_NOT_READY` means "not yet", not "failed".**

Read them in this order:

| Program | Shows |
|---|---|
| `httpget/` | The whole lifecycle for a request with no body; NOT_READY; offsets; EOF. |
| `httppost/` | A request body: `FN_OPEN_BODY_UNKNOWN`, `fn_write`, the zero-length commit. |
| `jsonq/` | Server-side JSON extraction with `fn_open_translated`. |
| `tcppipe/` | A permanent connection as a pipe: `FN_OPEN_KEEP`, framing, `fn_read_available`. |
| `waitany/` | Several sessions at once: `fn_ready`, and the hardware hint to wait without polling. |
| `httpbin/` | `fhttpbin`: a table-driven check of the HTTP client against httpbin, for regression testing. |

Test servers: `fujinet-nio/scripts/start_test_services.sh http` (httpbin on 8080) and `... tcp` (echo on 7777).
The 8-bit machines have no command line: build the address in with
`make TARGET=atari HTTPBIN_URL=http://<pc>:8080 TCP_URL=tcp://<pc>:7777`.
