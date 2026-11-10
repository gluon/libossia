#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 CNRS
# SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>
"""Tests of prism_map.py against fake OSCQuery servers on 127.0.0.1: they answer
?PEERS, ?PRISM_CONNECTIONS, both, or neither.

usage: python3 test_prism_map.py
"""
import gc
import http.server
import json
import math
import os
import re
import subprocess
import sys
import threading
import unittest
import warnings

HERE = os.path.dirname(os.path.abspath(__file__))
MAP = os.path.join(HERE, 'prism_map.py')


def fake_server(name, connections, peers=None):
    """An HTTP server answering ?HOST_INFO and, if connections or peers is not
    None, ?PRISM_CONNECTIONS or ?PEERS; started on a port the system chooses.
    It keeps the paths it was asked for in .asked."""
    class Handler(http.server.BaseHTTPRequestHandler):
        def do_GET(self):
            self.server.asked.append(self.path)
            ext = {'LISTEN': True, 'VALUE': True}
            if connections is not None:
                ext['PRISM_CONNECTIONS'] = True
            if peers is not None:
                ext['PEERS'] = True
            if self.path == '/?PEERS' and peers is not None:
                body = peers
            elif self.path.endswith('?PEERS'):
                self.send_response(404)
                self.end_headers()
                return
            elif self.path.endswith('?HOST_INFO'):
                body = {'NAME': name, 'OSC_PORT': self.server.server_port, 'EXTENSIONS': ext}
            elif self.path.endswith('?PRISM_CONNECTIONS') and connections is not None:
                body = connections
            elif self.path.endswith('?PRISM_CONNECTIONS'):
                self.send_response(400)
                self.end_headers()
                return
            else:
                body = {'FULL_PATH': '/', 'CONTENTS': {}}
            data = json.dumps(body).encode()
            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.send_header('Content-Length', str(len(data)))
            self.end_headers()
            self.wfile.write(data)

        def log_message(self, *args):
            pass

    srv = http.server.ThreadingHTTPServer(('127.0.0.1', 0), Handler)
    srv.asked = []
    threading.Thread(target=srv.serve_forever, daemon=True).start()
    return srv


class PrismMapTests(unittest.TestCase):
    def setUp(self):
        self.servers = []

    def tearDown(self):
        # Released here: a socket left open is a failure of this test
        with warnings.catch_warnings(record=True) as caught:
            warnings.simplefilter('always', ResourceWarning)
            while self.servers:
                s = self.servers.pop()
                s.shutdown()
                s.server_close()
                del s
            gc.collect()
        leaks = [str(w.message) for w in caught if issubclass(w.category, ResourceWarning)]
        self.assertEqual(leaks, [])

    def serve(self, name, connections, peers=None):
        s = fake_server(name, connections, peers)
        self.servers.append(s)
        return s.server_port

    def asked(self, port):
        return next(s.asked for s in self.servers if s.server_port == port)

    def run_map(self, *args):
        out = subprocess.run([sys.executable, MAP, '--no-browse', *args],
                             capture_output=True, text=True, timeout=30)
        self.assertEqual(out.returncode, 0, out.stderr)
        return out.stdout

    def test_clients_and_senders(self):
        a = self.serve('dev_a', {
            'CLIENTS': [
                {'IP': '127.0.0.1', 'PORT': 50000, 'NAME': 'max_patch', 'LISTEN': ['/level', '/speed'],
                 'PACKETS': 12},
                {'IP': '10.0.0.9', 'PORT': 50001, 'LISTEN': [], 'PACKETS': 0},
            ],
            'SENDERS': [{'IP': '127.0.0.1', 'PORT': 53012, 'PACKETS': 3, 'LAST_MS': 40}],
        })
        b = self.serve('dev_b', None)
        doc = json.loads(self.run_map('--host', f'127.0.0.1:{a}', '--host', f'127.0.0.1:{b}', '--json', '-'))

        devices = {d['name']: d for d in doc['devices']}
        self.assertEqual(set(devices), {'dev_a', 'dev_b'})
        self.assertTrue(devices['dev_a']['connections'])
        # A server that does not answer is listed, its connections unknown
        self.assertFalse(devices['dev_b']['connections'])

        edges = {(e['from'], e['to'], e['kind']): e for e in doc['edges']}
        follows = edges[('max_patch', 'dev_a', 'follows')]
        self.assertEqual(follows['listen'], ['/level', '/speed'])
        self.assertEqual(follows['packets'], 12)
        # A client with no name is named by its address
        self.assertIn(('10.0.0.9:50001', 'dev_a', 'follows'), edges)
        sends = edges[('127.0.0.1:53012', 'dev_a', 'sends')]
        self.assertEqual(sends['packets'], 3)
        self.assertEqual(len(doc['edges']), 3)

    def test_peers_is_read_when_the_server_declares_it(self):
        a = self.serve('dev_a',
                       {'CLIENTS': [{'IP': '127.0.0.1', 'PORT': 1, 'NAME': 'old', 'LISTEN': [], 'PACKETS': 0}],
                        'SENDERS': []},
                       {'CLIENTS': [{'IP': '127.0.0.1', 'PORT': 1, 'NAME': 'new', 'LISTEN': ['/x'], 'PACKETS': 2}],
                        'SENDERS': [{'IP': '127.0.0.1', 'PORT': 9, 'PACKETS': 1, 'LAST_MS': 5}]})
        doc = json.loads(self.run_map('--host', f'127.0.0.1:{a}', '--json', '-'))
        self.assertEqual({(e['from'], e['kind']) for e in doc['edges']}, {('new', 'follows'), ('127.0.0.1:9', 'sends')})
        self.assertTrue(doc['devices'][0]['connections'])
        self.assertIn('/?PEERS', self.asked(a))
        self.assertNotIn('/?PRISM_CONNECTIONS', self.asked(a))

    def test_peers_alone_is_enough(self):
        a = self.serve('dev_a', None, {'CLIENTS': [{'IP': '127.0.0.1', 'PORT': 1, 'NAME': 'new', 'LISTEN': [],
                                                    'PACKETS': 0}], 'SENDERS': []})
        doc = json.loads(self.run_map('--host', f'127.0.0.1:{a}', '--json', '-'))
        self.assertEqual([e['from'] for e in doc['edges']], ['new'])

    def test_prism_connections_is_read_when_peers_is_not_declared(self):
        a = self.serve('dev_a', {'CLIENTS': [{'IP': '127.0.0.1', 'PORT': 1, 'NAME': 'old', 'LISTEN': [],
                                              'PACKETS': 0}], 'SENDERS': []})
        doc = json.loads(self.run_map('--host', f'127.0.0.1:{a}', '--json', '-'))
        self.assertEqual([e['from'] for e in doc['edges']], ['old'])
        self.assertNotIn('/?PEERS', self.asked(a))

    def test_fields_omitted_by_peers_are_read_and_drawn(self):
        # ?PEERS omits what the server cannot fill: no "None:None" on the map
        a = self.serve('dev_a', None, {'CLIENTS': [{'LISTEN': [], 'PACKETS': 0}],
                                       'SENDERS': [{'IP': '127.0.0.1', 'PORT': 9, 'PACKETS': 1}]})
        doc = json.loads(self.run_map('--host', f'127.0.0.1:{a}', '--json', '-'))
        edges = {(e['from'], e['kind']): e for e in doc['edges']}
        self.assertIn(('unknown', 'follows'), edges)
        self.assertIsNone(edges[('127.0.0.1:9', 'sends')]['last_ms'])
        self.assertNotIn('None', self.run_map('--host', f'127.0.0.1:{a}', '--dot', '-'))
        self.assertNotIn('None:None', self.run_map('--host', f'127.0.0.1:{a}', '--html', '-'))
        self.run_map('--host', f'127.0.0.1:{a}')

    def test_dot_and_html(self):
        a = self.serve('dev_a', {'CLIENTS': [{'IP': '127.0.0.1', 'PORT': 1, 'NAME': 'td_client',
                                              'LISTEN': ['/x'], 'PACKETS': 0}], 'SENDERS': []})
        dot = self.run_map('--host', f'127.0.0.1:{a}', '--dot', '-')
        self.assertIn('"td_client" -> "dev_a"', dot)
        html = self.run_map('--host', f'127.0.0.1:{a}', '--html', '-')
        self.assertIn('<svg', html)
        self.assertIn('td_client', html)
        self.assertIn('dev_a', html)

    def test_an_edge_to_itself_is_drawn(self):
        # A client that gives the name of the server it follows: the edge goes
        # from a box to the same box, and has to be seen.
        a = self.serve('dev_a', {'CLIENTS': [{'IP': '127.0.0.1', 'PORT': 1, 'NAME': 'dev_a',
                                              'LISTEN': [], 'PACKETS': 10}], 'SENDERS': []})
        page = self.run_map('--host', f'127.0.0.1:{a}', '--html', '-')
        svg = page[page.index('</defs>'):page.index('</svg>')]
        extents = []
        for m in re.finditer(r'<line x1="(-?\d+)" y1="(-?\d+)" x2="(-?\d+)" y2="(-?\d+)"', svg):
            x1, y1, x2, y2 = map(float, m.groups())
            extents.append(math.hypot(x2 - x1, y2 - y1))
        for m in re.finditer(r'<path d="([^"]*)"', svg):
            n = [float(v) for v in re.findall(r'-?\d+(?:\.\d+)?', m.group(1))]
            xs, ys = n[0::2], n[1::2]
            extents.append(math.hypot(max(xs) - min(xs), max(ys) - min(ys)))
        self.assertEqual(len(extents), 1, svg)
        self.assertGreater(extents[0], 20, svg)
        self.assertIn('10 pkt', svg)

    def test_unreachable_host(self):
        doc = json.loads(self.run_map('--host', '127.0.0.1:1', '--json', '-'))
        self.assertEqual(doc['devices'][0]['error'] is not None, True)
        self.assertEqual(doc['edges'], [])


if __name__ == '__main__':
    unittest.main()
