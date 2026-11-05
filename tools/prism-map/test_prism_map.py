#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 CNRS
# SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>
"""Tests of prism_map.py against two fake OSCQuery servers on 127.0.0.1: one
answers ?PRISM_CONNECTIONS, the other does not know it.

usage: python3 test_prism_map.py
"""
import http.server
import json
import os
import subprocess
import sys
import threading
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
MAP = os.path.join(HERE, 'prism_map.py')


def fake_server(name, connections):
    """An HTTP server answering ?HOST_INFO and, if connections is not None,
    ?PRISM_CONNECTIONS; started on a port the system chooses."""
    class Handler(http.server.BaseHTTPRequestHandler):
        def do_GET(self):
            ext = {'LISTEN': True, 'VALUE': True}
            if connections is not None:
                ext['PRISM_CONNECTIONS'] = True
            if self.path.endswith('?HOST_INFO'):
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
    threading.Thread(target=srv.serve_forever, daemon=True).start()
    return srv


class PrismMapTests(unittest.TestCase):
    def setUp(self):
        self.servers = []

    def tearDown(self):
        for s in self.servers:
            s.shutdown()

    def serve(self, name, connections):
        s = fake_server(name, connections)
        self.servers.append(s)
        return s.server_port

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

    def test_dot_and_html(self):
        a = self.serve('dev_a', {'CLIENTS': [{'IP': '127.0.0.1', 'PORT': 1, 'NAME': 'td_client',
                                              'LISTEN': ['/x'], 'PACKETS': 0}], 'SENDERS': []})
        dot = self.run_map('--host', f'127.0.0.1:{a}', '--dot', '-')
        self.assertIn('"td_client" -> "dev_a"', dot)
        html = self.run_map('--host', f'127.0.0.1:{a}', '--html', '-')
        self.assertIn('<svg', html)
        self.assertIn('td_client', html)
        self.assertIn('dev_a', html)

    def test_unreachable_host(self):
        doc = json.loads(self.run_map('--host', '127.0.0.1:1', '--json', '-'))
        self.assertEqual(doc['devices'][0]['error'] is not None, True)
        self.assertEqual(doc['edges'], [])


if __name__ == '__main__':
    unittest.main()
