# Large tree bench

`LargeTreeBench.cpp`, built as `ossia_LargeTreeBench` with the tests
(`OSSIA_TESTING`), not run by ctest. It puts the client of ossia
(`oscquery_mirror_protocol`, what `ossia.client` and the mDNS directory of
ossia-max build) in front of a tree the size of a Live set of 64 tracks of
8 devices, without Live: a synthetic tree served by a test server on the
loopback. No mDNS, no Max, no other application.

## The tree

`/tracks/<t>`, each with 20 leaves and `devices/<d>` (8), each with 4 leaves
and `parameters/<p>` (48), each a container of 13 leaves. 4 tracks make the
base set, 64 are added by the build. Every leaf has `FULL_PATH`, `TYPE`,
`VALUE`, `ACCESS`, a `CLIPMODE` unless it is a string, and a `RANGE` for some.

| | This tree | LiveOSCQuery's load set (`docs/phase3.md`) |
| :- | -: | -: |
| Leaves | 342,992 | 340,414 |
| Containers | 27,338 | 25,534 |
| `GET /` | 46.6 MB | 47.4 MB |

`--tracks`, `--base-tracks`, `--devices`, `--parameters` change the size.

## The server

Beast on `127.0.0.1`, HTTP and WebSocket on one port. One network thread, and
one host thread that answers the readings one after the other, taking
`--host-ms-per-mb` (default 64) for each MB it answers: Live reads its values
on its own thread, one query at a time, and a full read of the load set takes
it 2.8 to 3.1 s. `--host-ms-per-mb 0` answers as fast as the tree is written,
as an ossia server would. The server counts the TCP connections it holds, the
readings under way, the WebSockets; `lsof` on the process counts the client's
established connections to the server's port, twice a second.

## Scenarios

- `tree`: the size of the tree.
- `read`: one mirror reads the whole tree, waited for without bound.
- `loop`: the connection of ossia-max to that device, by name or by URL: an
  `oscquery_connect_attempt` (`client_mirror.cpp`), then after a failure a new
  one `retry_ms` (1000) later, as `oscquery_directory::poll` and
  `client::attempt_check` do; a connection that holds is polled while its
  WebSocket is up. `--seconds` (60).
- `burst`: a mirror connected to the base tree (4 tracks, 20,176 leaves); the
  server adds a track every `--interval-ms` (900: 64 tracks in 58 s, Live
  built them in 61.5 s) and sends `PATH_CHANGED /tracks` after each, as
  LiveOSCQuery does.

`--json FILE` writes the result there too.

## Reference numbers

### Before the corrections

Branch `prism/bascule-876` at `bc8035c40`, macOS arm64 Release, on AC power,
three runs of `loop` and `burst`, defaults.

`read`, one run each:

| Server | Tree read after the WebSocket | Within the 3 s of `update()` |
| :- | -: | :- |
| 64 ms/MB | 3.54 s (3.23 s of it on the server) | no |
| 55 ms/MB | 3.10 s | no |
| 0 ms/MB | 0.60 s | yes |

`loop`, 60 s, three runs (the same figure in each unless a range is given):

| Measure | 64 ms/MB | 0 ms/MB (one run, 30 s) |
| :- | -: | -: |
| Attempts, connected | 15, 0 | 1, 1 |
| Seconds per attempt | 3.06 to 3.07 | 0.57 |
| Seconds between two attempts | 4.07 | |
| WebSocket open, seconds | 3.06 | 30.06 (to the end) |
| Readings of `/` never received by the client | 15 of 15 | 0 |
| Readings under way at once, at most | 2 | 1 |
| Client connections established at once (`lsof`), at most | 3 | |

`burst`, three runs:

| Measure | 64 ms/MB | 0 ms/MB (one run) |
| :- | -: | -: |
| `PATH_CHANGED /tracks` sent, readings of `/tracks` asked | 64, 64 | 64, 64 |
| Readings under way at once, at most | 24 | 2 |
| TCP connections the server held at once, at most | 25 | 3 |
| Client connections established at once (`lsof`), at most | 24 | 2 |
| TCP connections in all (64 readings, `HOST_INFO`, `/`, WebSocket) | 67 | 67 |
| Seconds per reading, p50, p95, max | 8.8, 62.2 to 62.4, 68.5 to 68.6 | 0.14, 0.25, 0.25 |
| Readings answered after the client gave them up (30 s) | 19 | 0 |
| After the build: tracks, leaves in the mirror, WebSocket | 68, 342,992, up | 68, 342,992, up |

`burst` then waited for one reading per `PATH_CHANGED` before it stopped;
since the corrections it stops after 2 s with no reading under way and none
added.

### After the corrections

`bf28fd1bb` (the attempt waits for the tree, a failed tree ends it) and
`3968dc45d` (one reading of a path at a time, two HTTP queues), measured at
`7f5bc4813`, same machine, on AC power, three runs of `loop` and `burst`
(remeasured: the first figures, taken on battery, differed by 0.02 s at most).

| Measure | 64 ms/MB | 0 ms/MB (one run) |
| :- | -: | -: |
| `read`: tree read after the WebSocket | 3.48 s | 0.53 s |
| `read`: resident memory of the client, before and with the tree | 6 MB, 577 MB | 6 MB, 577 MB |
| `loop` (60 s): attempts, connected | 1, 1 (at 3.49 to 3.51 s) | |
| `loop`: WebSocket open, readings of `/` | 60.06 to 60.07 s (to the end), 1 | |
| `loop` (600 s, one run): connected, WebSocket open | at 3.51 s, 600.06 s (to the end) | |
| `loop`: HTTP connections the server held at once; client connections (`lsof`), WebSocket included | 2; 2 | |
| `burst`: `PATH_CHANGED /tracks` sent, readings of `/tracks` | 64, 40 | 64, 64 |
| `burst`: readings under way at once, at most | 2 | 2 |
| `burst`: HTTP connections the server held at once, at most | 2 | 2 |
| `burst`: client connections established at once (`lsof`), WebSocket included | 2 to 3 | 2 |
| `burst`: seconds per reading, p50, p95, max | 1.17 to 1.18, 2.92, 3.20 to 3.21 | 0.14, 0.24, 0.25 |
| `burst`: readings answered after the client gave them up | 0 | 0 |
| `burst`: after the build: tracks, leaves in the mirror, WebSocket | 68, 342,992, up | 68, 342,992, up |

At most two client HTTP connections at a time, one per queue: the readings of
the tree, and the rest (values, `HOST_INFO`, commands). In `loop` and `burst`
the two at once are the `HOST_INFO` and the tree of the connection; `lsof`,
sampled twice a second, saw both beside the WebSocket in one `burst` run of
three (3 connections), at most one of them in the others.
