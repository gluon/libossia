#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 CNRS
# SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>
"""A Minuit device that comes does not make Max announce services of its own,
and one that leaves the network does not freeze Max, checked in a running Max
with the package installed, no click.

usage: python3 tests/Max/check_minuit.py --may-freeze [--max <Max.app>] [--out <folder>]

With a package that has the defect (ossia 2.1.0 to ossia-prism 2.1.0+prism.10),
this test FREEZES MAX: its scheduler stops for good, and its main thread too
when it enters Max's critical region (opening a patcher does); Max has to be
killed and started again. It refuses to run without --may-freeze, or while a
bench of TouchDesigner is running.

Opens check_minuit.maxpat (a metro that counts on UDP 41742; it closes itself).
dns-sd announces a device _minuit._tcp; the listener of the package, which every
ossia object starts, must hold it (get devices); 5 s later, dns-sd -B must see
that one service and no other; dns-sd withdraws it. Nothing connects to it:
loading the package is enough. The metro has to go on: a scheduler that stops
stops it. A frozen Max is sampled into --out. It refuses to run while
_minuit._tcp services are on the network.

With a package whose listener's devices announce themselves (ossia 2.1.0 to
ossia-prism 2.1.0+prism.11 before the fix), Max announces services of its own,
more and more, until it quits: quit Max after a failure. The patcher also
takes /connect <name>, to connect its ossia.client by hand.
"""
import argparse
import os
import socket
import struct
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPLY, CONTROL = 41742, 41743
NAME = 'prism11_gone'
results = []


def check(name, ok, detail=''):
    results.append(bool(ok))
    print(f'{"PASS" if ok else "FAIL"} {name}' + (f': {detail}' if detail != '' else ''), flush=True)


def pad(b):
    return b + b'\0' * (4 - len(b) % 4)


def parse(d):
    e = d.index(b'\0')
    address, i = d[:e].decode(), (e // 4 + 1) * 4
    e = d.index(b'\0', i)
    tags, i = d[i + 1:e].decode(), (e // 4 + 1) * 4
    values = []
    for t in tags:
        if t in 'if':
            values.append(struct.unpack('>' + t, d[i:i + 4])[0])
            i += 4
        elif t == 's':
            e = d.index(b'\0', i)
            values.append(d[i:e].decode())
            i = (e // 4 + 1) * 4
    return address, values


def bench_running():
    out = subprocess.run(['ps', '-axo', 'command'], capture_output=True, text=True).stdout
    return [l[:70] for l in out.splitlines()
            if ('bidir_run.py' in l or 'load_run' in l or 'Contents/MacOS/TouchDesigner' in l) and 'grep' not in l]


def minuit_services(seconds=4):
    """The _minuit._tcp services on the network, as dns-sd -B sees them."""
    try:
        out = subprocess.run(['dns-sd', '-B', '_minuit._tcp', 'local'], capture_output=True,
                             text=True, stdin=subprocess.DEVNULL, timeout=seconds).stdout
    except subprocess.TimeoutExpired as e:
        out = (e.stdout or b'').decode() if isinstance(e.stdout, bytes) else (e.stdout or '')
    names = set()
    for line in out.splitlines():
        f = line.split(None, 6)
        if len(f) == 7 and f[1] in ('Add', 'Rmv') and f[5] == '_minuit._tcp.':
            (names.add if f[1] == 'Add' else names.discard)(f[6])
    return sorted(names)


class Patch:
    def __init__(self):
        self.tx = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.rx = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.rx.bind(('127.0.0.1', REPLY))
        self.rx.settimeout(0.05)

    def send(self, address, text=None):
        d = pad(address.encode()) + (pad(b',s') + pad(text.encode()) if text else pad(b','))
        self.tx.sendto(d, ('127.0.0.1', CONTROL))

    def read(self, seconds):
        start = last = time.time()
        beats, other, end = 0, [], start + seconds
        self.gap = 0.0
        while time.time() < end:
            try:
                a, v = parse(self.rx.recv(1024))
            except socket.timeout:
                continue
            if a == '/beat':
                beats += 1
                now = time.time()
                self.gap = max(self.gap, now - last)
                last = now
            else:
                other.append((a, v))
        self.gap = max(self.gap, time.time() - last)
        return beats, other


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--may-freeze', action='store_true')
    ap.add_argument('--max', default='Max')
    ap.add_argument('--out', default='.')
    a = ap.parse_args()
    if not a.may_freeze:
        sys.exit('this test freezes a Max with the defect: see the header; --may-freeze')
    running = bench_running()
    if running:
        sys.exit(f'a bench is running, not now: {running}')
    before = minuit_services()
    if before:
        sys.exit(f'_minuit._tcp services on the network already, the count would be wrong: {before[:3]}')
    p = Patch()
    subprocess.run(['open', '-a', a.max, os.path.join(HERE, 'check_minuit.maxpat')], check=True)
    beats, _ = p.read(6)
    check('the metro of the patch counts', beats > 10, f'{beats} in 6 s')
    if beats <= 10:
        sys.exit(1)

    dns = subprocess.Popen(['dns-sd', '-R', f'{NAME} Minuit server', '_minuit._tcp', 'local', '9998'],
                           stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    beats, _ = p.read(3)
    # Not checked: the listener resolves each instance on the scheduler, and its
    # own clients announce themselves in turn; the metro may wait about 1 s.
    print(f'INFO announced: the metro: {beats} in 3 s, longest gap {p.gap:.2f} s', flush=True)
    p.send('/devices')
    beats, seen = p.read(2)
    listed = [v for x, v in seen if x == '/client' and v[:2] == ['device', 'minuit']]
    check(f'the listener holds {NAME}', any(NAME in str(v[2]) for v in listed if len(v) > 2),
          str(listed[:4]))
    beats, _ = p.read(5)
    services = minuit_services()
    check('one announcement, one service: the devices of the listener do not announce themselves',
          services == [f'{NAME} Minuit server'], f'{len(services)} services: {services[:3]}')
    if len(services) > 1:
        print('INFO Max announces services of its own: they go on until Max quits', flush=True)

    dns.terminate()
    dns.wait(5)
    print(f'INFO {NAME} withdrawn at {time.strftime("%H:%M:%S")}', flush=True)
    beats, _ = p.read(6)
    check('withdrawn: the metro goes on, Max is not frozen', beats > 30,
          f'{beats} in 6 s, longest gap {p.gap:.2f} s')
    if beats <= 30:
        pid = subprocess.run(['pgrep', '-x', 'Max'], capture_output=True, text=True).stdout.split()
        if pid:
            path = os.path.join(a.out, 'minuit-frozen-sample.txt')
            subprocess.run(['sample', pid[0], '2', '-file', path], capture_output=True, timeout=30)
            print(f'INFO Max {pid[0]} is frozen: sample in {path}. It has to be restarted.', flush=True)
    else:
        p.send('/close')
    print(f'RESULT: {"PASS" if all(results) else "FAIL"} ({sum(results)}/{len(results)})')
    sys.exit(0 if all(results) else 1)


if __name__ == '__main__':
    main()
