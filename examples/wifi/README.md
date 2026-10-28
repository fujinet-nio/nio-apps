# wifi - the adapter's Wi-Fi link

Status, configuration and scanning. Both examples use caller-owned buffers (the library allocates nothing),
and `wifitest` is read-only, so it is safe to run on any adapter.

| Program | Shows |
|---|---|
| `wifitest/` | The smallest use of status, config and scan; a good smoke test for a new target. |
| `wificonfig/` | An end-to-end configuration exercise through the public API, including a small caller-owned scan buffer. |
