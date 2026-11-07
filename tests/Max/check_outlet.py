#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 CNRS
# SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>
"""The outlet of ossia.parameter against the value its device serves, when a
value of the inlet and a value of the network come at the same time, checked
in a running Max with the package installed, no click.

usage: python3 tests/Max/check_outlet.py [--max <Max.app>] [--trials <n>]

Opens check_outlet.maxpat with open -a (Max is never quit; the patch closes
itself): ossia.device prism11_outlet on 7431 (OSC) and 7432 (HTTP, WebSocket).
It holds Max's main thread for a moment in a script: not to run during a bench
that uses Max. The same instant is made sure, not waited for: the script holds
the main thread, the network value comes meanwhile, then the script writes.

Three observers: /direct, what leaves the outlet, sent by udpsend on the thread
that outputs it; /js_rx, what a script receives of it; ?VALUE, what the device
serves. A script that writes can end on another value than the device, the
limit written in the README: this measures it, and checks its workaround. The network value comes over UDP, then over the WebSocket (needs node).
Prints one line per check, then RESULT: PASS or RESULT: FAIL.
"""
import argparse
import json
import os
import socket
import struct
import subprocess
import sys
import time
import urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
OSC, HTTP, REPLY, CONTROL = 7431, 7432, 41740, 41741
results = []


def check(name, ok, detail=''):
    results.append(bool(ok))
    print(f'{"PASS" if ok else "FAIL"} {name}' + (f': {detail}' if detail != '' else ''), flush=True)


def pad(b):
    return b + b'\0' * (4 - len(b) % 4)


def osc(address, *values):
    tags = ''.join('f' if isinstance(v, float) else 'i' for v in values)
    data = pad(address.encode()) + pad((',' + tags).encode())
    for v in values:
        data += struct.pack('>f', v) if isinstance(v, float) else struct.pack('>i', v)
    return data


def parse(d):
    e = d.index(b'\0')
    address, i = d[:e].decode(), (e // 4 + 1) * 4
    e = d.index(b'\0', i)
    tags, i = d[i + 1:e].decode(), (e // 4 + 1) * 4
    values = []
    for t in tags:
        if t in 'fi':
            values.append(round(struct.unpack('>' + t, d[i:i + 4])[0], 3))
            i += 4
    return address, values


def served():
    with urllib.request.urlopen(f'http://127.0.0.1:{HTTP}/level?VALUE', timeout=10) as r:
        return round(json.loads(r.read().decode().rstrip('\0'))['VALUE'], 3)


# A client over the WebSocket: each line on its stdin is a value it writes
WS_JS = r'''
const ws = new WebSocket("ws://127.0.0.1:%d");
ws.binaryType = "arraybuffer";
const pad = (s) => { const b = Buffer.from(s); return Buffer.concat([b, Buffer.alloc(4 - b.length %% 4)]); };
ws.onopen = () => console.log("open");
ws.onerror = () => { console.log("error"); process.exit(1); };
let rest = "";
process.stdin.on("data", (d) => {
  rest += d;
  let i;
  while ((i = rest.indexOf("\n")) >= 0) {
    const f = Buffer.alloc(4);
    f.writeFloatBE(parseFloat(rest.slice(0, i)));
    rest = rest.slice(i + 1);
    ws.send(Buffer.concat([pad("/level"), pad(",f"), f]));
  }
});
process.stdin.on("end", () => process.exit(0));
''' % HTTP


class Bench:
    def __init__(self):
        self.tx = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.rx = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.rx.bind(('127.0.0.1', REPLY))
        self.rx.settimeout(0.02)
        self.ws = None

    def open_ws(self):
        self.ws = subprocess.Popen(['node', '-e', WS_JS], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True)
        return self.ws.stdout.readline().strip() == 'open'

    def close_ws(self):
        self.ws.stdin.close()
        self.ws.wait(5)
        self.ws = None

    def control(self, address, *values):
        self.tx.sendto(osc(address, *values), ('127.0.0.1', CONTROL))

    def network(self, value):
        if self.ws:
            self.ws.stdin.write(f'{value}\n')
            self.ws.stdin.flush()
        else:
            self.tx.sendto(osc('/level', float(value)), ('127.0.0.1', OSC))

    def read(self, seconds):
        """(time, address, values) of what the patch sends for that long"""
        got, end = [], time.time() + seconds
        while time.time() < end:
            try:
                d = self.rx.recv(1024)
            except socket.timeout:
                continue
            got.append((time.time(), *parse(d)))
        return got

    def wait_ready(self, seconds):
        return any(a == '/ready' for _, a, _ in self.read(seconds))

    def trial(self, write, net, hold=0.08, after=0.02, settle=0.5):
        """The script holds the main thread for hold s, then writes `write`;
        the network value leaves `after` s after the hold began"""
        t0 = time.time()
        self.control('/go', float(hold * 1000), float(write))
        time.sleep(after)
        self.network(net)
        got = self.read(settle)
        direct = [v[0] for _, a, v in got if a == '/direct']
        js = [v[0] for _, a, v in got if a == '/js_rx']
        return {'direct': direct, 'js': js, 'served': served(),
                'times': {a: round(t - t0, 3) for t, a, _ in got}, 'all': got, 't0': t0}


def bench_running():
    out = subprocess.run(['ps', '-axo', 'command'], capture_output=True, text=True).stdout
    return [l[:70] for l in out.splitlines()
            if ('bidir_run.py' in l or 'load_run' in l or 'Contents/MacOS/TouchDesigner' in l) and 'grep' not in l]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--max', default='Max')
    ap.add_argument('--trials', type=int, default=20)
    a = ap.parse_args()
    if bench_running():
        sys.exit(f'a bench is running, not now: {bench_running()}')
    b = Bench()
    subprocess.run(['open', '-a', a.max, os.path.join(HERE, 'check_outlet.maxpat')], check=True)
    if not b.wait_ready(15):
        sys.exit('check_outlet.maxpat did not answer')
    time.sleep(1)

    # Which thread: the network value while the script holds the main thread
    b.control('/slow', 0)
    b.control('/go', 400.0, -1.0)
    t0 = time.time()
    time.sleep(0.05)
    b.network(111.0)
    got = b.read(1.0)
    direct = next((t - t0 for t, x, _ in got if x == '/direct'), None)
    rx = next((t - t0 for t, x, _ in got if x == '/js_rx'), None)
    end = next((t - t0 for t, x, _ in got if x == '/js_end'), None)
    print(f'INFO main thread held until {end}; the outlet at {direct}, the script received it at {rx}', flush=True)
    on_its_own_thread = direct is not None and end is not None and direct < end
    print(f'INFO the network value is output {"while the main thread is held: on another thread" if on_its_own_thread else "after the main thread: on the main thread"}', flush=True)

    # The same instant, what the bench of TouchDesigner met in 1 of 2600
    def series(label, base, settle=0.5, after=0.02):
        apart_out, apart_js, lost, order = 0, 0, 0, {}
        for i in range(a.trials):
            write, net = base + i, base + 5000.0 + i
            r = b.trial(write, net, after=after, settle=settle)
            key = ' '.join('network' if v == net else 'inlet' if v == write else '?' for v in r['direct'])
            order[key] = order.get(key, 0) + 1
            s = r['served']
            apart_out += not r['direct'] or r['direct'][-1] != s
            apart_js += bool(r['js']) and r['js'][-1] != s
            lost += s != write
            if i == 0:
                print(f'INFO {label}, first trial: inlet {write}, network {net}: outlet {r["direct"]}, script {r["js"]}, served {s}', flush=True)
        print(f'INFO {label}: order at the outlet {order}; the device ends on the network value {lost} times', flush=True)
        check(f'{label}: the outlet ends on the value the device serves', apart_out == 0, f'{apart_out} of {a.trials} apart')
        print(f'INFO {label}: a script ends apart from the device {apart_js} of {a.trials} times (the limit)', flush=True)
        return lost, apart_js

    _, apart = series('same instant, over UDP', 1000.0)
    b.control('/workaround', 1)
    _, apart = series('with the workaround, a bang when a value comes', 3000.0)
    check('with the workaround: a script ends on the value the device serves', apart == 0, f'{apart} of {a.trials} apart')
    b.control('/workaround', 0)
    if b.open_ws():
        series('same instant, over the WebSocket', 2000.0)
        b.close_ws()
    else:
        check('the WebSocket client connects', False)

    # A consumer on the outlet that takes its time: the inlet writes meanwhile
    b.control('/slow', 1)
    took = 0
    for count in (200000, 400000, 800000, 1600000, 3200000):
        b.control('/count', count)
        time.sleep(0.05)
        b.network(float(100 + count % 97))
        got = b.read(2.5)
        first = next((t for t, x, _ in got if x == '/direct'), None)
        done = next((t for t, x, _ in got if x == '/slow_done'), None)
        took = done - first if first and done else 0
        if took > 0.15:
            break
    print(f'INFO consumer of {took * 1000:.0f} ms ({count} loops)', flush=True)
    lost, _ = series('consumer that takes its time', 8000.0, settle=max(1.0, took * 4), after=0.01)
    check('consumer that takes its time: the value of the inlet is not lost', lost == 0, f'{lost} of {a.trials} lost, without a message')
    b.control('/slow', 0)

    b.control('/close')
    print(f'RESULT: {"PASS" if all(results) else "FAIL"} ({sum(results)}/{len(results)})')
    sys.exit(0 if all(results) else 1)


if __name__ == '__main__':
    main()
