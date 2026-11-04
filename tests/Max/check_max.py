#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 CNRS
# SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>
"""The defects of ossia-prism fixed in Max for 2.1.0+prism.10, checked in a
running Max with the package installed, no click.

usage: python3 tests/Max/check_max.py [--max <Max.app>]

Opens check_device.maxpat (prism10_check, 7417 and 7418), check_local.maxpat
(prism10_local on 127.0.0.1, 7419 and 7420, and a second expose on 7417 and
7418), check_global.maxpat ([ossia], 7421 and 7422), then check_client.maxpat,
with open -a: Max is never quit. The patches close themselves (40 s, 12 s for
the client one). They send their counts back to UDP 41730; nothing listens
on 41739. Needs node (WebSocket) for the
bundle check. Prints one line per check, then RESULT: PASS or RESULT: FAIL.
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
results = []


def check(name, ok, detail=''):
    results.append(ok)
    print(f'{"PASS" if ok else "FAIL"} {name}' + (f': {detail}' if detail else ''), flush=True)


def get(port, path):
    with urllib.request.urlopen(f'http://127.0.0.1:{port}{path}', timeout=2) as r:
        return json.loads(r.read().decode().rstrip('\0'))


def osc(address, tags='', *values):
    def pad(b):
        return b + b'\0' * (4 - len(b) % 4)
    data = pad(address.encode()) + pad((',' + tags).encode())
    for t, v in zip(tags, values):
        data += struct.pack('>f', v) if t == 'f' else struct.pack('>i', v)
    return data


def send_udp(port, data):
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
        s.sendto(data, ('127.0.0.1', port))


def other_ipv4():
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
            s.connect(('192.0.2.1', 9))
            ip = s.getsockname()[0]
            return None if ip.startswith('127.') else ip
    except OSError:
        return None


def connects(ip, port):
    try:
        with socket.create_connection((ip, port), timeout=0.5):
            return True
    except OSError:
        return False


def announced(seconds=2.5):
    try:
        p = subprocess.Popen(['dns-sd', '-B', '_oscjson._tcp', 'local.'], stdout=subprocess.PIPE,
                             stderr=subprocess.DEVNULL, stdin=subprocess.DEVNULL, text=True)
    except OSError:
        return None
    time.sleep(seconds)
    p.terminate()
    return p.communicate()[0]


BUNDLE_JS = r'''
const ws = new WebSocket("ws://127.0.0.1:7422");
ws.binaryType = "arraybuffer";
let frames = 0, messages = 0;
const pad = (s) => { const b = Buffer.from(s); return Buffer.concat([b, Buffer.alloc(4 - b.length % 4)]); };
ws.onopen = () => {
  ws.send(JSON.stringify({COMMAND: "LISTEN", DATA: "/ba"}));
  ws.send(JSON.stringify({COMMAND: "LISTEN", DATA: "/bb"}));
  setTimeout(() => ws.send(Buffer.concat([pad("/bgo"), pad(",")])), 300);
  setTimeout(() => { console.log(JSON.stringify({frames, messages})); process.exit(0); }, 1300);
};
ws.onmessage = (ev) => {
  if (typeof ev.data === "string") return;
  frames++;
  const b = Buffer.from(ev.data);
  messages += b.subarray(0, 7).toString() === "#bundle" ? b.toString("latin1").split("/b").length - 1 : 1;
};
ws.onerror = () => { console.log(JSON.stringify({error: true})); process.exit(1); };
'''


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--max', default='Max')
    a = ap.parse_args()

    # The counts the patches send back
    sink = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sink.bind(('127.0.0.1', 41730))
    sink.settimeout(0.5)
    counts = {}

    def drain(until):
        while time.time() < until:
            try:
                data = sink.recv(1024)
            except socket.timeout:
                continue
            counts[data.split(b'\0', 1)[0].decode()] = struct.unpack('>i', data[-4:])[0]

    for name in ('check_device.maxpat', 'check_local.maxpat', 'check_global.maxpat'):
        subprocess.run(['open', '-a', a.max, os.path.join(HERE, name)], check=True)
        drain(time.time() + 2)
    drain(time.time() + 3)

    # The bool served as a JSON boolean
    try:
        v = get(7418, '/flag?VALUE')
        check('bool served as true or false', v.get('VALUE') is False, json.dumps(v))
    except Exception as e:
        check('bool served as true or false', False, str(e))
    # A vec3f bounded by @min 0 @max 1: three ranges
    try:
        r = get(7418, '/pos').get('RANGE')
        ok = isinstance(r, list) and len(r) == 3 and all(x.get('MIN') == 0 and x.get('MAX') == 1 for x in r)
        check('vec3f @min 0 @max 1: a range per component', ok, json.dumps(r))
    except Exception as e:
        check('vec3f @min 0 @max 1: a range per component', False, str(e))
    # dump on ossia.remote does not write a value
    try:
        v = get(7418, '/dumpme?VALUE').get('VALUE')
        check('dump on ossia.remote writes no value', v == 0.5, f'VALUE {v!r}')
    except Exception as e:
        check('dump on ossia.remote writes no value', False, str(e))
    # A second expose on the same ports: expose 0
    dup = counts.get('/check/dup_expose')
    check('second server on the same ports reports it', dup == 0, f'expose {dup!r}')
    # ?PRISM_CONNECTIONS
    try:
        info = get(7418, '/?HOST_INFO')
        conn = get(7418, '/?PRISM_CONNECTIONS')
        check('?PRISM_CONNECTIONS from ossia.device',
              info['EXTENSIONS'].get('PRISM_CONNECTIONS') is True and 'CLIENTS' in conn, json.dumps(conn)[:120])
    except Exception as e:
        check('?PRISM_CONNECTIONS from ossia.device', False, str(e))
    # expose oscquery 7419 7420 127.0.0.1
    other = other_ipv4()
    check('expose on 127.0.0.1: reached there', connects('127.0.0.1', 7420))
    if other:
        check('expose on 127.0.0.1: refused through another interface', not connects(other, 7420))
    names = announced()
    if names is not None:
        check('expose on 127.0.0.1: not announced, the others are',
              'prism10_local' not in names and 'prism10_check' in names)

    # @hold 500: a value of the network while the inlet holds the control
    send_udp(7417, osc('/go'))
    time.sleep(0.05)
    send_udp(7417, osc('/lvl', 'f', 0.9))
    time.sleep(0.2)
    during = get(7418, '/lvl_out?VALUE').get('VALUE')
    time.sleep(0.8)
    after = get(7418, '/lvl_out?VALUE').get('VALUE')
    check('@hold 500: held, then out', during != 0.9 and abs((after or 0) - 0.9) < 1e-6,
          f'{during!r} at 250 ms, {after!r} at 1050 ms')

    # [ossia] @bundle 1: ba and bb set in one tick arrive in one frame
    try:
        out = subprocess.run(['node', '-e', BUNDLE_JS], capture_output=True, text=True, timeout=10).stdout
        r = json.loads(out.strip().splitlines()[-1])
        check('[ossia] @bundle 1: one frame for a tick', r.get('frames') == 1 and r.get('messages') == 2, out.strip())
    except Exception as e:
        check('[ossia] @bundle 1: one frame for a tick', False, str(e))

    # The client patch sends its counts back
    subprocess.run(['open', '-a', a.max, os.path.join(HERE, 'check_client.maxpat')], check=True)
    end = time.time() + 15
    while time.time() < end and not all(k in counts for k in (
            '/check/named_outputs', '/check/reconnect0_connects', '/check/byname_tree_changes')):
        drain(time.time() + 1)
    sink.close()
    named = counts.get('/check/named_outputs')
    check('[ossia.client <name>] does not loop', named == 0, f'{named} outputs in 6 s')
    rec = counts.get('/check/reconnect0_connects')
    check('@reconnect 0: a failed first connection is not tried again', rec == 1, f'{rec} connect in 6 s')
    churn = counts.get('/check/byname_tree_changes')
    check('connect <name>: the tree is not read again', churn == 0, f'{churn} parameter/node messages')

    print(f'RESULT: {"PASS" if all(results) else "FAIL"} ({sum(results)}/{len(results)})')
    sys.exit(0 if all(results) else 1)


if __name__ == '__main__':
    main()
