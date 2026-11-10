# prism_map.py: who follows whom

Reads, once, the OSCQuery devices of a network and draws which client follows which server. The network is often static: run it when you need the picture, there is no monitoring loop.

```bash
python3 prism_map.py --html map.html
```

Python 3.8 or later, standard library only.

## What it reads

1. The devices announced over mDNS (`_oscjson._tcp`), found with `dns-sd` for `--browse-ms` milliseconds (2000 by default). `dns-sd` ships with macOS; elsewhere, or for a device that is not announced (a server on `127.0.0.1` only), give it with `--host HOST:PORT`, repeatable, and `--no-browse` to skip the browsing.
2. The `HOST_INFO` of each device.
3. `?PEERS` of each device that declares it in the `EXTENSIONS` of its `HOST_INFO` (`oscquery` 0.4.0 and `2.1.0+prism.12`), or else `?PRISM_CONNECTIONS` of each device that declares that. The servers of ossia-prism `2.1.0+prism.10` and later declare it (Max `ossia.device`, Unity `Controller`), and the server of TouchDesigner's OSCQuery component (prism-td-oscquery) from `oscquery` 0.3.3. A device that does not is listed, with its connections unknown.

`?PEERS` has the same keys; its `SENDERS` come most recent first and it omits a field the server cannot fill (a client with neither name nor address is shown as `unknown`).

`?PRISM_CONNECTIONS` is a PRISM extension of OSCQuery. It answers on any path, as `HOST_INFO`:

```json
{
  "CLIENTS": [
    {"IP": "127.0.0.1", "PORT": 56451, "NAME": "max_patch", "OSC_PORT": 10203,
     "SENDER_PORT": 61680, "LISTEN": ["/level"], "PACKETS": 12}
  ],
  "SENDERS": [
    {"IP": "127.0.0.1", "PORT": 57654, "PACKETS": 2, "LAST_MS": 211}
  ]
}
```

| Key | Meaning |
|---|---|
| `CLIENTS` | the WebSocket clients of the server |
| `IP`, `PORT` | where the client connects from |
| `NAME` | the name the client gave with the text command `{"COMMAND": "PRISM_NAME", "DATA": "<name>"}`; absent otherwise. To a server that declares `PRISM_CONNECTIONS`, the Max package gives `max_client_<process>` for the connection it opens to each announced device, which `connect <name>` reuses (`2.1.0+prism.11`; before, the name of the device it connected to), and the name of the `ossia.client` for a connection by URL; `Ossia.Client` gives `unity_client_<id>_<attempt>` |
| `OSC_PORT`, `SENDER_PORT` | the UDP ports the client receives on and sends from, when it streams OSC over UDP |
| `LISTEN` | the addresses the client asked for with `LISTEN` |
| `PACKETS` | the OSC packets the server received from that client, datagrams or WebSocket frames |
| `SENDERS` | the UDP sources that are none of the clients, the last 256: `IP`, `PORT`, `PACKETS`, `LAST_MS` since their last packet |

## Outputs

- No option: a text summary.
- `--json FILE`: the devices and the edges. An edge `follows` goes from a client to a server, with its `listen` and `packets`; an edge `sends` from a UDP source to a server. A client is named by its `NAME`, else by `IP:PORT`.
- `--dot FILE`: the graph for Graphviz (`dot -Tsvg`).
- `--html FILE`: one page with the graph drawn in SVG and the table of the devices, no script.

`-` writes to the standard output.

## Limits

- A client that gives no name is known by its address only; several devices on one machine share it.
- `LISTEN` is what a client asked for. The libossia server still sends every value to every client: `LISTEN` does not filter yet.
- A source that writes over UDP without a WebSocket is a sender, whatever it is.
- The browsing needs `dns-sd`; without it, use `--host`.

## Test

```bash
python3 test_prism_map.py
```

Fake servers on `127.0.0.1`, that answer `?PEERS`, `?PRISM_CONNECTIONS`, both or neither: the edges read, the Graphviz and HTML outputs, an edge from a device to itself drawn as a loop in the HTML (since `2.1.0+prism.11`; a line had no length), an unreachable host. A test that leaves a socket open fails.
