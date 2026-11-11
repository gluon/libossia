#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 CNRS
# SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>
"""Two red lines of Max's console, checked in a running Max with the package
installed, no click.

usage: python3 tests/Max/check_console.py [--max <Max.app>]

Opens check_console.maxpat with open -a (Max is never quit; the patch closes
itself): the device check_console, OSCQuery on 41761 (OSC) and 41762 (HTTP,
WebSocket), and the client check_console_client.

- 200 WebSocket clients open their connection and reset it at once. Up to
  2.1.0+prism.12, each one could put "oscquery_server_protocol::
  on_connectionOpen: remote_endpoint: Invalid argument" in red. ?PEERS keeps
  no client without an address, and a client that stays is listed with its
  address and port.
- The client connects to a server of this script on 41763, which then sends
  it a close frame and keeps the connection. Up to 2.1.0+prism.12, the
  disconnect of the client put "Error when stopping WS server" in red and
  held Max until the server ended the connection, 5 s.

The console itself is not read: look at it for these two lines after the run.
Refuses to run while a bench of TouchDesigner is running. Prints one line per
check, then RESULT: PASS or RESULT: FAIL.
"""
import argparse
import base64
import hashlib
import json
import socket
import struct
import subprocess
import sys
import os
import threading
import time
import urllib.request

sys.dont_write_bytecode = True
from check_minuit import bench_running, pad, parse  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
OSC, WS = 41761, 41762
FAKE, FAKE_OSC = 41763, 41764
REPLY, CONTROL = 41765, 41766
results = []


def check(name, ok, detail=''):
    results.append(bool(ok))
    print(f'{"PASS" if ok else "FAIL"} {name}' + (f': {detail}' if detail != '' else ''), flush=True)


def until(f, seconds):
    end = time.time() + seconds
    while time.time() < end:
        try:
            if f():
                return True
        except OSError:
            pass
        time.sleep(0.05)
    return False


def attribute():
    """?PEERS, or ?PRISM_CONNECTIONS on a device that does not declare it
    (before 2.1.0+prism.12): asking for an attribute the device does not know
    puts a red line in the console"""
    with urllib.request.urlopen(f'http://127.0.0.1:{WS}/?HOST_INFO', timeout=2) as r:
        extensions = json.loads(r.read().decode().rstrip('\0')).get('EXTENSIONS', {})
    return 'PEERS' if extensions.get('PEERS') else 'PRISM_CONNECTIONS'


def peers():
    with urllib.request.urlopen(f'http://127.0.0.1:{WS}/?{ATTRIBUTE[0]}', timeout=2) as r:
        return json.loads(r.read().decode().rstrip('\0')).get('CLIENTS', [])


ATTRIBUTE = ['PEERS']


def handshake(port):
    return ('GET / HTTP/1.1\r\nHost: 127.0.0.1:%d\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n'
            'Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 13\r\n\r\n' % port).encode()


def opened(port):
    s = socket.create_connection(('127.0.0.1', port), timeout=2)
    s.sendall(handshake(port))
    answer = b''
    while b'\r\n\r\n' not in answer:
        chunk = s.recv(512)
        if not chunk:
            break
        answer += chunk
    return s


def open_and_reset(port, read_answer):
    s = opened(port) if read_answer else socket.create_connection(('127.0.0.1', port), timeout=2)
    if not read_answer:
        s.sendall(handshake(port))
    s.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, struct.pack('ii', 1, 0))
    s.close()


class ClosingServer:
    """An OSCQuery server reduced to one parameter: it answers HTTP, and on
    close() sends its WebSocket clients a close frame and keeps them"""
    ROOT = json.dumps({'FULL_PATH': '/', 'ACCESS': 0, 'CONTENTS': {
        'f': {'FULL_PATH': '/f', 'TYPE': 'f', 'ACCESS': 3, 'VALUE': [1.0]}}})
    HOST_INFO = json.dumps({'NAME': 'check_console_server', 'OSC_PORT': FAKE_OSC, 'OSC_TRANSPORT': 'UDP',
                            'EXTENSIONS': {'VALUE': True, 'LISTEN': True}})

    def __init__(self):
        self.ws, self.lock = [], threading.Lock()
        self.listener = socket.socket()
        self.listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.listener.bind(('127.0.0.1', FAKE))
        self.listener.listen(16)
        threading.Thread(target=self.accept, daemon=True).start()

    def accept(self):
        while True:
            try:
                c, _ = self.listener.accept()
            except OSError:
                return
            threading.Thread(target=self.serve, args=(c,), daemon=True).start()

    def serve(self, c):
        request = b''
        while b'\r\n\r\n' not in request:
            chunk = c.recv(4096)
            if not chunk:
                c.close()
                return
            request += chunk
        head = request.split(b'\r\n\r\n')[0].decode(errors='replace')
        fields = {l.split(':', 1)[0].strip().lower(): l.split(':', 1)[1].strip()
                  for l in head.split('\r\n')[1:] if ':' in l}
        if fields.get('upgrade', '').lower() == 'websocket':
            accept = base64.b64encode(hashlib.sha1(
                (fields['sec-websocket-key'] + '258EAFA5-E914-47DA-95CA-C5AB0DC85B11').encode()).digest()).decode()
            c.sendall(('HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n'
                       'Sec-WebSocket-Accept: %s\r\n\r\n' % accept).encode())
            with self.lock:
                self.ws.append(c)
            # What the client sends is read and dropped; the connection is kept
            try:
                while c.recv(4096):
                    pass
            except OSError:
                pass
            return
        path = head.split(' ')[1] if ' ' in head else '/'
        body = (self.HOST_INFO if 'HOST_INFO' in path else self.ROOT if path.split('?')[0] == '/' else '').encode()
        status = '200 OK' if body else '204 No Content'
        c.sendall(('HTTP/1.1 %s\r\nContent-Type: application/json\r\nContent-Length: %d\r\n'
                   'Connection: close\r\n\r\n' % (status, len(body))).encode() + body)
        c.close()

    def clients(self):
        with self.lock:
            return len(self.ws)

    def close(self):
        # Close, 1001: going away; the TCP connection stays
        with self.lock:
            for c in self.ws:
                c.sendall(bytes([0x88, 0x02, 0x03, 0xE9]))


class Patch:
    def __init__(self):
        self.tx = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.rx = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.rx.bind(('127.0.0.1', REPLY))
        self.rx.settimeout(0.02)

    def send(self, address):
        self.tx.sendto(pad(address.encode()) + pad(b','), ('127.0.0.1', CONTROL))

    def read(self, seconds, until_address=None):
        got, end = [], time.time() + seconds
        while time.time() < end:
            try:
                got.append(parse(self.rx.recv(1024)))
                if until_address and got[-1][0] == until_address:
                    break
            except socket.timeout:
                pass
        return got


def run(max_app, patch, fake):
    subprocess.run(['open', '-a', max_app, os.path.join(HERE, 'check_console.maxpat')], check=True)
    ready = any(a == '/ready' for a, _ in patch.read(15, '/ready'))
    check('the patch is open', ready)
    if not ready:
        return
    def known():
        ATTRIBUTE[0] = attribute()
        return True
    answers = until(known, 5)
    check(f'the device answers on {WS}, its clients by ?{ATTRIBUTE[0]}', answers)
    if not answers:
        return

    # Clients that leave as they connect
    before = peers()
    for i in range(200):
        open_and_reset(WS, i % 2 == 0)
    time.sleep(1)
    after = peers()
    nameless = [c for c in after if not c.get('IP') or not c.get('PORT')]
    check(f'200 clients reset at once: ?{ATTRIBUTE[0]} has no client without an address', not nameless, str(nameless))
    check(f'?{ATTRIBUTE[0]} keeps the clients it had', len(after) == len(before), f'{len(before)} then {len(after)}')
    stay = opened(WS)
    port = stay.getsockname()[1]
    listed = until(lambda: any(c.get('IP') == '127.0.0.1' and c.get('PORT') == port for c in peers()), 3)
    check('a client that stays is listed with its address and port', listed, str(peers()))
    stay.close()

    # A client of Max stopped while its server closes the connection
    patch.send('/connect')
    connected = until(lambda: fake.clients() > 0, 10)
    got = patch.read(1)
    check('check_console_client connected to the server of the script', connected,
          str([x for x in got if x[0] == '/client'][:3]))
    if not connected:
        return
    fake.close()
    time.sleep(0.3)
    t0 = time.time()
    patch.send('/disconnect')
    done = any(a == '/disconnected' for a, _ in patch.read(10, '/disconnected'))
    held = time.time() - t0
    check('the disconnect of the client returns', done)
    check('the disconnect of the client does not hold Max', done and held < 1, f'{held:.2f} s')
    patch.send('/close')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--max', default='/Applications/Max.app')
    args = ap.parse_args()
    running = bench_running()
    if running:
        print(f'a bench of TouchDesigner is running: {running}', flush=True)
        return 2
    fake, patch = ClosingServer(), Patch()
    run(args.max, patch, fake)
    print('look at the console of Max: no "on_connectionOpen" nor "Error when stopping WS server"', flush=True)
    ok = results and all(results)
    print(f'RESULT: {"PASS" if ok else "FAIL"} ({sum(results)}/{len(results)})', flush=True)
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
