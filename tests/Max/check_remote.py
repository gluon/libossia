#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 CNRS
# SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>
"""An ossia.remote gets the value of a node the server adds while its client
is connected once, then once for each change, by URL and by name, checked in a
running Max with the package installed, no click.

usage: python3 tests/Max/check_remote.py --tests <ossia_V8ConnectorTest> [--max <Max.app>] [--out <folder>]

The server is a libossia server in a process of its own: the test binary of
tests/Network/V8ConnectorTest.cpp, run as zz_added_node_server, device
prism11_added on 7441 (OSC) and 7442 (HTTP, WebSocket), announced over mDNS.
Opens check_remote.maxpat with open -a (Max is never quit; the patch closes
itself): two ossia.client, prism11_url connected by URL and prism11_byname
connected by name, each with an ossia.remote bench/late, which sends what it
outputs to UDP 41744. Then the server adds /bench/late with 0.5, set without a
push, and pushes 0.75. With ossia-prism 2.1.0+prism.10 the remote output the
added value twice. What the server receives and sends is written to
--out/server.log. Refuses to run while a bench of TouchDesigner is running.
Prints one line per check, then RESULT: PASS or RESULT: FAIL.
"""
import argparse
import json
import os
import socket
import subprocess
import sys
import time
import urllib.request

sys.dont_write_bytecode = True
from check_minuit import bench_running, pad, parse  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
REPLY, CONTROL = 41744, 41745
OSC, WS = 7441, 7442
NAME = 'prism11_added'
results = []


def check(name, ok, detail=''):
    results.append(bool(ok))
    print(f'{"PASS" if ok else "FAIL"} {name}' + (f': {detail}' if detail != '' else ''), flush=True)


def clients():
    with urllib.request.urlopen(f'http://127.0.0.1:{WS}/?PRISM_CONNECTIONS', timeout=2) as r:
        return [c.get('NAME', '') for c in json.loads(r.read().decode().rstrip('\0')).get('CLIENTS', [])]


def until(f, seconds):
    end = time.time() + seconds
    while time.time() < end:
        try:
            if f():
                return True
        except OSError:
            pass
        time.sleep(0.1)
    return False


class Server:
    def __init__(self, tests, log):
        env = dict(os.environ, OSSIA_BENCH_OSC=str(OSC), OSSIA_BENCH_WS=str(WS), OSSIA_BENCH_NAME=NAME,
                   OSSIA_BENCH_LOG=log)
        self.p = subprocess.Popen([tests, 'zz_added_node_server'], env=env, text=True,
                                  stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

    def send(self, command):
        self.p.stdin.write(command + '\n')
        self.p.stdin.flush()
        while True:
            line = self.p.stdout.readline()
            if not line:
                sys.exit(f'the server stopped on "{command}"')
            if line.startswith('done '):
                return line.split()[1]

    def stop(self):
        try:
            self.p.stdin.write('quit\n')
            self.p.stdin.close()
            self.p.wait(5)
        except (OSError, subprocess.TimeoutExpired):
            self.p.kill()
            self.p.wait()


class Patch:
    def __init__(self):
        self.tx = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.rx = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.rx.bind(('127.0.0.1', REPLY))
        self.rx.settimeout(0.05)

    def send(self, address):
        self.tx.sendto(pad(address.encode()) + pad(b','), ('127.0.0.1', CONTROL))

    def read(self, seconds):
        got, end = [], time.time() + seconds
        while time.time() < end:
            try:
                got.append(parse(self.rx.recv(1024)))
            except socket.timeout:
                pass
        return got


def values(got, address):
    return [round(v[0], 4) if isinstance(v[0], float) else v for a, v in got if a == address and v]


def run(server, patch, max_app, max_pid):
    if not until(lambda: clients() is not None, 10):
        check(f'the server answers on {WS}', False)
        return
    subprocess.run(['open', '-a', max_app, os.path.join(HERE, 'check_remote.maxpat')], check=True)
    ready = any(a == '/ready' for a, _ in patch.read(10))
    check('the patch is open', ready)
    if not ready:
        return
    # The listener of the package connects to every announced device
    found = until(lambda: len(clients()) > 0, 20)
    check(f'Max found {NAME} by name', found, str(clients()))
    if not found:
        return
    named = f'max_client_{max_pid}'
    check(f'its connection is named {named}', until(lambda: named in clients(), 3), str(clients()))
    patch.send('/connect')
    connected = until(lambda: 'prism11_url' in clients(), 10)
    got = patch.read(2)
    check('the clients connected', connected,
          f'{clients()}; {[x for x in got if x[0].endswith("_client")][:4]}')
    if not connected:
        return
    before = patch.read(1)

    server.send('add 0.5')
    added = patch.read(2)
    server.send('push 0.75')
    pushed = patch.read(2)
    dumps = [x for x in before + added + pushed if x[0].endswith('_dump')]
    if dumps:
        print(f'INFO dumpout of the remotes: {dumps[:6]}', flush=True)
    for how, address in (('by URL', '/url'), ('by name', '/name')):
        check(f'{how}: nothing before the node is added', values(before, address) == [],
              str(values(before, address)))
        check(f'{how}: the added node, its value once', values(added, address) == [0.5],
              str(values(added, address)))
        check(f'{how}: one change, one value', values(pushed, address) == [0.75],
              str(values(pushed, address)))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--tests', required=True, help='the ossia_V8ConnectorTest binary')
    ap.add_argument('--max', default='Max')
    ap.add_argument('--out', default='.')
    a = ap.parse_args()
    running = bench_running()
    if running:
        sys.exit(f'a bench is running, not now: {running}')
    max_pid = subprocess.run(['pgrep', '-x', 'Max'], capture_output=True, text=True).stdout.split()
    if not max_pid:
        sys.exit('Max is not running')

    log = os.path.join(a.out, 'server.log')
    if os.path.exists(log):
        sys.exit(f'{log} exists: one folder per run')
    server = Server(a.tests, os.path.abspath(log))
    patch = Patch()
    try:
        run(server, patch, a.max, max_pid[0])
    finally:
        patch.send('/close')
        server.stop()
    print(f'RESULT: {"PASS" if results and all(results) else "FAIL"} ({sum(results)}/{len(results)})')
    sys.exit(0 if results and all(results) else 1)


if __name__ == '__main__':
    main()
