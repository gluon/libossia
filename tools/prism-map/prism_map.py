#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 CNRS
# SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>
"""Who follows whom among the OSCQuery devices of a network, read once.

usage: python3 prism_map.py [--host HOST:PORT ...] [--browse-ms MS] [--no-browse]
                            [--json FILE] [--dot FILE] [--html FILE]

Finds the devices announced over mDNS (_oscjson._tcp, through dns-sd, on
macOS) and the ones given with --host, reads the HOST_INFO of each, then
?PEERS from those that declare it in EXTENSIONS, or else ?PRISM_CONNECTIONS
from those that declare that: the libossia
servers of ossia-prism (Max ossia.device, Unity Controller) since
2.1.0+prism.10, and the TouchDesigner OSCQuery component (prism-td-oscquery)
since oscquery 0.3.3. A device that declares neither is listed, its
connections unknown.

Each client of a server becomes an edge "follows" from the client to the
server, with the addresses it listens to and the OSC packets it sent; a
client is named by the name it gave, else by IP:PORT, else "unknown": ?PEERS
omits what a server cannot fill. Each UDP source that
is none of the clients becomes an edge "sends". FILE "-" writes to the
standard output. Without any output option, prints a text summary.
Python 3.8 or later, standard library only.
"""
import argparse
import html
import json
import math
import re
import subprocess
import sys
import time
import urllib.error
import urllib.request


def browse(ms):
    """[(name, host, port)] of the devices announced over mDNS, through
    dns-sd; [] where dns-sd is missing."""
    def run(args, seconds):
        try:
            p = subprocess.Popen(['dns-sd', *args], stdout=subprocess.PIPE,
                                 stderr=subprocess.DEVNULL, stdin=subprocess.DEVNULL, text=True)
        except OSError:
            return ''
        time.sleep(seconds)
        p.terminate()
        return p.communicate()[0]

    names = []
    for line in run(['-B', '_oscjson._tcp', 'local.'], ms / 1000).splitlines():
        m = re.search(r'\sAdd\s+\S+\s+\S+\s+\S+\s+_oscjson\._tcp\.\s+(.*)$', line)
        if m and m.group(1).strip() not in names:
            names.append(m.group(1).strip())
    found = []
    for name in names:
        out = run(['-L', name, '_oscjson._tcp', 'local.'], 1.0)
        m = re.search(r'can be reached at (\S+?)\.?:(\d+)', out)
        if m:
            found.append((name, m.group(1), int(m.group(2))))
    return found


def get_json(host, port, query, timeout=2.0):
    with urllib.request.urlopen(f'http://{host}:{port}/{query}', timeout=timeout) as r:
        return json.loads(r.read().decode('utf-8').rstrip('\0'))


def read_device(host, port, announced=None):
    d = {'name': announced or f'{host}:{port}', 'host': host, 'port': port,
         'connections': False, 'error': None, 'clients': [], 'senders': []}
    try:
        info = get_json(host, port, '?HOST_INFO')
        d['name'] = info.get('NAME') or d['name']
        extensions = info.get('EXTENSIONS', {})
        query = next((q for q in ('PEERS', 'PRISM_CONNECTIONS') if extensions.get(q)), None)
        if query:
            conn = get_json(host, port, '?' + query)
            d['connections'] = True
            d['clients'] = conn.get('CLIENTS', [])
            d['senders'] = conn.get('SENDERS', [])
    except (OSError, ValueError, urllib.error.URLError) as e:
        d['error'] = str(e)
    return d


def where(entry):
    """IP:PORT of a client or a UDP source, or "unknown" if the server omitted it."""
    return f"{entry['IP']}:{entry['PORT']}" if 'IP' in entry and 'PORT' in entry else 'unknown'


def edges_of(devices):
    edges = []
    for d in devices:
        for c in d['clients']:
            edges.append({'from': c.get('NAME') or where(c), 'to': d['name'],
                          'kind': 'follows', 'listen': c.get('LISTEN', []), 'packets': c.get('PACKETS', 0)})
        for s in d['senders']:
            edges.append({'from': where(s), 'to': d['name'], 'kind': 'sends',
                          'packets': s.get('PACKETS', 0), 'last_ms': s.get('LAST_MS')})
    return edges


def to_dot(doc):
    lines = ['digraph oscquery {', '  rankdir=LR;']
    for d in doc['devices']:
        style = 'solid' if d['connections'] else 'dashed'
        lines.append(f'  "{d["name"]}" [shape=box, style={style}];')
    for e in doc['edges']:
        if e['kind'] == 'follows':
            label = f'{len(e["listen"])} LISTEN, {e["packets"]} packets'
            lines.append(f'  "{e["from"]}" -> "{e["to"]}" [label="{label}"];')
        else:
            lines.append(f'  "{e["from"]}" -> "{e["to"]}" [style=dashed, label="UDP {e["packets"]}"];')
    lines.append('}')
    return '\n'.join(lines) + '\n'


def to_html(doc):
    """One page, no script: devices on a circle, clients and senders around."""
    servers = [d['name'] for d in doc['devices']]
    others = sorted({e['from'] for e in doc['edges']} - set(servers))
    nodes = servers + others
    w, h = 900, 640
    cx, cy, r = w / 2, h / 2, min(w, h) / 2 - 110
    pos = {}
    for i, n in enumerate(nodes):
        a = 2 * math.pi * i / max(1, len(nodes)) - math.pi / 2
        pos[n] = (cx + r * math.cos(a), cy + r * math.sin(a))
    svg = [f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {w} {h}" width="100%">',
           '<defs><marker id="arrow" viewBox="0 0 10 10" refX="10" refY="5" markerWidth="8" '
           'markerHeight="8" orient="auto"><path d="M0,0 L10,5 L0,10 z" fill="currentColor"/></marker></defs>']
    for e in doc['edges']:
        (x1, y1), (x2, y2) = pos[e['from']], pos[e['to']]
        dash = ' stroke-dasharray="6 4"' if e['kind'] == 'sends' else ''
        label = (f'{len(e["listen"])} LISTEN' if e['kind'] == 'follows' else 'UDP') + f', {e["packets"]} pkt'
        title = f'<title>{html.escape(", ".join(e.get("listen", [])))}</title>'
        if e['from'] == e['to']:
            # From a box to itself: a loop above it, a line would have no length
            svg.append(f'<path d="M{x1 - 24:.0f},{y1 - 18:.0f} C{x1 - 60:.0f},{y1 - 80:.0f} '
                       f'{x1 + 60:.0f},{y1 - 80:.0f} {x1 + 24:.0f},{y1 - 18:.0f}" fill="none" '
                       f'stroke="currentColor"{dash} marker-end="url(#arrow)">{title}</path>')
            lx, ly = x1, y1 - 72
        else:
            dx, dy = x2 - x1, y2 - y1
            n = math.hypot(dx, dy) or 1
            x2, y2 = x2 - dx / n * 60, y2 - dy / n * 22
            svg.append(f'<line x1="{x1:.0f}" y1="{y1:.0f}" x2="{x2:.0f}" y2="{y2:.0f}" stroke="currentColor"'
                       f'{dash} marker-end="url(#arrow)">{title}</line>')
            lx, ly = (x1 + x2) / 2, (y1 + y2) / 2 - 6
        svg.append(f'<text x="{lx:.0f}" y="{ly:.0f}" font-size="11" '
                   f'text-anchor="middle">{html.escape(label)}</text>')
    for n in nodes:
        x, y = pos[n]
        server = n in servers
        svg.append(f'<rect x="{x - 60:.0f}" y="{y - 18:.0f}" width="120" height="36" rx="6" '
                   f'fill="{"#dbe8ff" if server else "#f0f0f0"}" stroke="currentColor"/>')
        svg.append(f'<text x="{x:.0f}" y="{y + 4:.0f}" font-size="12" text-anchor="middle">{html.escape(n)}</text>')
    svg.append('</svg>')
    rows = ''.join(f'<tr><td>{html.escape(d["name"])}</td><td>{html.escape(d["host"])}:{d["port"]}</td>'
                   f'<td>{"yes" if d["connections"] else "no"}</td><td>{html.escape(d["error"] or "")}</td></tr>'
                   for d in doc['devices'])
    return ('<!doctype html><html><head><meta charset="utf-8"><title>OSCQuery map</title>'
            '<style>body{font-family:sans-serif;margin:16px}table{border-collapse:collapse}'
            'td,th{border:1px solid #ccc;padding:4px 8px}</style></head><body>'
            f'<h1>OSCQuery map</h1><p>Read {html.escape(doc["read_at"])}.</p>{"".join(svg)}'
            '<table><tr><th>Device</th><th>Address</th><th>Connections</th><th>Error</th></tr>'
            f'{rows}</table></body></html>\n')


def write(path, text):
    if path == '-':
        sys.stdout.write(text)
    else:
        with open(path, 'w', encoding='utf-8') as f:
            f.write(text)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--host', action='append', default=[], help='HOST:PORT of a device, repeatable')
    ap.add_argument('--browse-ms', type=int, default=2000, help='mDNS browsing time (default 2000)')
    ap.add_argument('--no-browse', action='store_true', help='only the --host devices')
    ap.add_argument('--json')
    ap.add_argument('--dot')
    ap.add_argument('--html')
    a = ap.parse_args()

    targets = []
    if not a.no_browse:
        targets += browse(a.browse_ms)
    for hp in a.host:
        host, _, port = hp.rpartition(':')
        targets.append((None, host, int(port)))
    devices = [read_device(host, port, name) for name, host, port in targets]
    doc = {'read_at': time.strftime('%Y-%m-%d %H:%M:%S'), 'devices': devices, 'edges': edges_of(devices)}

    if a.json:
        write(a.json, json.dumps(doc, indent=2) + '\n')
    if a.dot:
        write(a.dot, to_dot(doc))
    if a.html:
        write(a.html, to_html(doc))
    if not (a.json or a.dot or a.html):
        for d in devices:
            state = d['error'] or ('connections read' if d['connections'] else 'no connections declared')
            print(f'{d["name"]} ({d["host"]}:{d["port"]}): {state}')
        for e in doc['edges']:
            what = f'listens to {len(e["listen"])}' if e['kind'] == 'follows' else 'sends over UDP'
            print(f'  {e["from"]} -> {e["to"]}: {what}, {e["packets"]} packets')


if __name__ == '__main__':
    main()
