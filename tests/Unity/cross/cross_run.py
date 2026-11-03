#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 CNRS
# SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>
"""Cross bench of the Unity package with Max and TouchDesigner, no click.

usage: python3 cross_run.py <run folder> --package <io.ossia.ossia-unity-*.tgz>
                            --td-project <folder> --td-tests <folder>
                            [--unity-app <Unity.app>] [--td <TouchDesigner.app>] [--max <Max.app>]

The run folder is outside the repository: the Unity project and its player
go there, and every log. --td-project is a TouchDesigner project folder with
prism_oscquery.tox, prism_oscquery_server.tox, .venv/ and
TDPyEnvManagerContext.yaml (the Installation of prism-td-components);
--td-tests is oscquery/tests of prism-td-components, whose docs_run.py
writes the TouchDesigner project that runs cross_td.py.

Pairs, each with values both ways, the echo counted both ways (one side
writes 20 values, 50 ms apart, away from any change of the other; nothing
may come back) and 20 crossed writes (both sides write a different value in
the same instant; both must end on the same value):
- Unity serves unity_cross: Max (ossia.client), TouchDesigner
  (prism_oscquery) and a second Unity (Ossia.Client) follow it, by address,
  then by name;
- Unity (Ossia.Client) follows Max (ossia.device max_cross) and
  TouchDesigner (prism_oscquery_server td_cross), by address, then by name;
- Unity plays the scene of the sample "Follow a device", which follows
  follow_me, the device of FollowADevice.maxpat, by name, then by address;
  Max reads and writes follow_me as another client.

Max needs the package ossia-prism installed; TouchDesigner must not be
running. Prints one report merging every log in time order, and
RESULT: PASS or RESULT: FAIL; exit code 0 on PASS.
"""
import argparse
import glob
import json
import os
import plistlib
import shutil
import socket
import struct
import subprocess
import sys
import tarfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
MAX_PORT, TD_PORT, UNITY_PORTS = 7493, 7494, {'unity1': 7495, 'unity2': 7497}
MAX_REPLY, TEXT_REPLY = 7496, 7498
UNITY_SERVER = ('unity_cross', 9870, 9871)
MAX_DEVICE = ('max_cross', 9880, 9881)
TD_SERVER = ('td_cross', 9890, 9891)
FOLLOW = ('follow_me', 9678)
ECHO_N, ECHO_MS = 20, 0.05
CROSS_N, CROSS_WAIT = 20, 0.6
PROBES = ['CrossFields.cs', 'CrossProbe.cs']
SAMPLE = 'package/Samples~/FollowADevice/'

START_TEXT = '''# Written by tests/Unity/cross/cross_run.py: runs cross_td.py when the project starts.
SCRIPT = {script!r}
OUT = {out!r}

def onStart():
	import time, traceback
	g = dict(globals())
	g.update(__file__=SCRIPT, __name__='cross_td', CROSS_OUT=OUT)
	try:
		exec(compile(open(SCRIPT).read(), SCRIPT, 'exec'), g)
	except Exception:
		with open(OUT + '/td.log', 'a') as f:
			f.write('%.3f FAIL cross_td.py did not start: %s\\n' % (time.time(), traceback.format_exc().replace('\\n', ' | ')))
	return
'''

OUT = None
results = {'PASS': 0, 'FAIL': 0}


def log(text):
    line = '%.3f %s' % (time.time(), text)
    with open(os.path.join(OUT, 'runner.log'), 'a') as f:
        f.write(line + '\n')
    print('  ' + text, flush=True)


def check(name, ok, got=''):
    verdict = 'PASS' if ok else 'FAIL'
    results[verdict] += 1
    log(f'{verdict} {name}' + (f': {got}' if got != '' else ''))
    return ok


def same(a, b):
    try:
        return abs(float(a) - float(b)) < 1e-3
    except (TypeError, ValueError):
        return False


# -- links: OSC with Max, text with TouchDesigner and Unity -------------------

def pad(b):
    return b + b'\0' * (4 - len(b) % 4)


def osc(address, *args):
    tags, data = ',', b''
    for a in args:
        if isinstance(a, bool) or isinstance(a, int):
            tags, data = tags + 'i', data + struct.pack('>i', int(a))
        elif isinstance(a, float):
            tags, data = tags + 'f', data + struct.pack('>f', a)
        else:
            tags, data = tags + 's', data + pad(str(a).encode())
    return pad(address.encode()) + pad(tags.encode()) + data


def unosc(data):
    end = data.index(b'\0')
    address = data[:end].decode()
    i = (end // 4 + 1) * 4
    end = data.index(b'\0', i)
    tags = data[i + 1:end].decode()
    i = (end // 4 + 1) * 4
    args = []
    for t in tags:
        if t == 'i':
            args.append(struct.unpack('>i', data[i:i + 4])[0])
            i += 4
        elif t == 'f':
            args.append(struct.unpack('>f', data[i:i + 4])[0])
            i += 4
        elif t == 's':
            end = data.index(b'\0', i)
            args.append(data[i:end].decode())
            i = (end // 4 + 1) * 4
    return address, args


def bound(port):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.bind(('127.0.0.1', port))
    s.setblocking(False)
    return s


class Links:
    def __init__(self):
        self.max_sock, self.text_sock = bound(MAX_REPLY), bound(TEXT_REPLY)
        self.max_in, self.text_in = [], []

    def poll(self):
        for s, box, decode in ((self.max_sock, self.max_in, unosc),
                               (self.text_sock, self.text_in, lambda d: d.decode().split())):
            while True:
                try:
                    data, _ = s.recvfrom(65536)
                except (BlockingIOError, OSError):
                    break
                try:
                    box.append(decode(data))
                except Exception:
                    pass

    def max(self, address, *args):
        self.max_sock.sendto(osc(address, *args), ('127.0.0.1', MAX_PORT))

    def text(self, peer, *words):
        port = TD_PORT if peer == 'td' else UNITY_PORTS[peer]
        self.text_sock.sendto(' '.join(str(w) for w in words).encode(), ('127.0.0.1', port))

    def take_max(self, address, first=None):
        self.poll()
        for k, (a, args) in enumerate(self.max_in):
            if a == address and (first is None or (args and args[0] == first)):
                del self.max_in[k]
                return args or ['']   # a reply without arguments is still a reply
        return None

    def take_text(self, peer, word):
        self.poll()
        for k, parts in enumerate(self.text_in):
            if parts[:2] == [peer, word]:
                del self.text_in[k]
                return parts[2:] or ['']
        return None


L = None


def listening(port):
    """True when something of this machine listens on TCP port."""
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(0.5)
    try:
        return s.connect_ex(('127.0.0.1', port)) == 0
    finally:
        s.close()


def udp_taken(port):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.bind(('127.0.0.1', port))
        return False
    except OSError:
        return True
    finally:
        s.close()


def until(fn, timeout, step=0.05):
    end = time.time() + timeout
    while True:
        got = fn()
        if got or time.time() > end:
            return got
        time.sleep(step)


def ask_text(peer, words, reply, timeout=3):
    L.take_text(peer, reply)
    L.text(peer, *words)
    return until(lambda: L.take_text(peer, reply), timeout)


def ask_max(address, args, reply, first=None, timeout=3):
    L.max(address, *args)
    return until(lambda: L.take_max(reply, first), timeout)


# -- the peers, one interface ------------------------------------------------

class Unity:
    def __init__(self, tag):
        self.tag, self.name = tag, f'Unity ({tag})'

    def set(self, v):
        L.text(self.tag, 'set', v)

    def read(self):
        got = ask_text(self.tag, ['read'], 'value')
        return float(got[0]) if got and got[0] != '-' else None

    def reset(self):
        ask_text(self.tag, ['reset'], 'reset')

    def back(self):
        got = ask_text(self.tag, ['count'], 'count')
        return int(got[0]) - int(got[1]) if got else None


class Max:
    def __init__(self, role):
        self.role, self.name = role, f'Max ({role})'

    def set(self, v):
        L.max('/cross/put', self.role, 'value', float(v))

    def counted(self):
        got = ask_max('/cross/count', [self.role, 'value'], '/cross/counted', self.role)
        return got

    def read(self):
        got = self.counted()
        return float(got[3]) if got and got[3] != '-' else None

    def reset(self):
        L.max('/cross/reset')
        time.sleep(0.1)

    def back(self):
        got = self.counted()
        return int(got[2]) if got else None


class TD:
    def __init__(self, role):
        self.role, self.name = role, f'TouchDesigner ({role})'
        self.base = None

    def set(self, v):
        L.text('td', 'set', v)

    def read(self):
        got = ask_text('td', ['read'], 'value')
        return float(got[0]) if got and got[0] != '-' else None

    def stats(self):
        got = ask_text('td', ['stats'], 'stats')
        return [int(x) for x in got] if got else None

    def reset(self):
        self.base = self.stats()

    def back(self):
        now = self.stats()
        if now is None or self.base is None:
            return None
        received, _, echoes = (a - b for a, b in zip(now, self.base))
        # what was applied as new, and the copies of its own messages dropped
        return received + echoes


# -- the checks --------------------------------------------------------------

def both_ways(pair, a, b, k):
    for writer, reader, v in ((a, b, 11.0 + k), (b, a, 21.0 + k)):
        writer.set(v)
        got = until(lambda: same(reader.read(), v), 3, 0.2)
        check(f'{pair}: {writer.name} writes {v:g}, {reader.name} reads it', bool(got), f'{reader.read()}')


def echo(pair, writer, reader, k):
    """The writer writes ECHO_N values, away from any change of the other:
    the reader holds the last, and nothing comes back to the writer."""
    time.sleep(1)
    writer.reset()
    last = None
    for i in range(ECHO_N):
        last = 600.0 + 100 * k + i
        writer.set(last)
        time.sleep(ECHO_MS)
    got = until(lambda: same(reader.read(), last), 3, 0.2)
    time.sleep(1)
    back = writer.back()
    log(f'INFO echo {pair}: {writer.name} wrote {ECHO_N} values, {reader.name} holds {reader.read()}, '
        f'{back} came back to {writer.name}')
    check(f'echo {pair}, {writer.name} writes: {reader.name} holds the last value', bool(got), f'{reader.read()}')
    check(f'echo {pair}, {writer.name} writes: nothing comes back', back == 0, f'{back} values')


def cross(pair, server, client):
    diverged, wins = [], {server.name: 0, client.name: 0}
    for i in range(CROSS_N):
        s, c = 2000.0 + i, 3000.0 + i
        server.set(s)
        client.set(c)
        time.sleep(CROSS_WAIT)
        at_server, at_client = server.read(), client.read()
        if at_server is None or not same(at_server, at_client):
            diverged.append(f'{i}: {server.name} {at_server}, {client.name} {at_client}')
        else:
            wins[server.name if same(at_server, s) else client.name] += 1
    log(f'INFO cross {pair}: {CROSS_N} crossed writes; both ended on the value of {server.name} '
        f'{wins[server.name]} times, of {client.name} {wins[client.name]} times; diverged {len(diverged)} times'
        + (f' ({"; ".join(diverged)})' if diverged else ''))
    check(f'cross {pair}: both end on the same value after each of {CROSS_N} crossed writes', not diverged,
          f'{len(diverged)} diverged')


def pair_checks(pair, server, client, k):
    both_ways(pair, server, client, k)
    echo(pair, client, server, k)
    echo(pair, server, client, k + 50)
    cross(pair, server, client)


# -- Unity project and player ------------------------------------------------

def find_unity():
    found = sorted(glob.glob('/Applications/Unity/Hub/Editor/6000.3.*/Unity.app'))
    if not found:
        sys.exit('no Unity 6000.3 in /Applications/Unity/Hub/Editor: name it with --unity-app')
    return found[-1]


def unity_batch(app, logfile, *args):
    exe = os.path.join(app, 'Contents', 'MacOS', 'Unity')
    return subprocess.run([exe, '-batchmode', '-quit', '-logFile', logfile, *args], stdin=subprocess.DEVNULL,
                          timeout=1800).returncode


def prepare_player(app, folder, tgz):
    """The project, created if missing, with the package, the probe and the
    sample "Follow a device" copied as the Package Manager imports it; its
    player built again when the package or a probe file is newer."""
    project = os.path.join(folder, 'unity_cross')
    player = os.path.join(project, 'Build', 'cross.app')
    if not os.path.exists(os.path.join(project, 'ProjectSettings')):
        print(f'Unity: creating the project {project}', flush=True)
        if unity_batch(app, os.path.join(folder, 'unity_create.log'), '-createProject', project):
            sys.exit('Unity could not create the project: see unity_create.log')
    manifest = os.path.join(project, 'Packages', 'manifest.json')
    m = json.load(open(manifest))
    if m['dependencies'].get('io.ossia.ossia-unity') != 'file:' + tgz:
        m['dependencies']['io.ossia.ossia-unity'] = 'file:' + tgz
        json.dump(m, open(manifest, 'w'), indent=2)
    assets = os.path.join(project, 'Assets', 'Cross')
    os.makedirs(os.path.join(assets, 'Editor'), exist_ok=True)
    inputs = [tgz, os.path.join(HERE, 'unity', 'Editor', 'CrossBuild.cs')]
    for f in PROBES:
        shutil.copy(os.path.join(HERE, 'unity', f), assets)
        inputs.append(os.path.join(HERE, 'unity', f))
    shutil.copy(os.path.join(HERE, 'unity', 'Editor', 'CrossBuild.cs'), os.path.join(assets, 'Editor'))
    sample = os.path.join(project, 'Assets', 'Samples', 'FollowADevice')
    os.makedirs(sample, exist_ok=True)
    with tarfile.open(tgz) as t:
        for member in t.getmembers():
            if member.isfile() and member.name.startswith(SAMPLE) and '/' not in member.name[len(SAMPLE):]:
                with open(os.path.join(sample, member.name[len(SAMPLE):]), 'wb') as f:
                    f.write(t.extractfile(member).read())
    stamp = os.path.join(project, 'Build', 'cross.built')
    built = os.path.getmtime(stamp) if os.path.exists(player) and os.path.exists(stamp) else 0
    if all(os.path.getmtime(f) < built for f in inputs):
        print(f'Unity: player up to date, {player}', flush=True)
        return player, sample
    print(f'Unity: building the player {player} (a few minutes)', flush=True)
    logfile = os.path.join(folder, 'unity_build.log')
    if unity_batch(app, logfile, '-projectPath', project, '-executeMethod', 'CrossBuild.Build',
                   '-crossPlayer', player) or not os.path.exists(player):
        sys.exit(f'the player was not built: see {logfile}')
    os.makedirs(os.path.dirname(stamp), exist_ok=True)
    open(stamp, 'w').close()
    return player, sample


def launch(player, tag):
    exe = os.path.join(player, 'Contents', 'MacOS', 'cross')
    return subprocess.Popen([exe, '-batchmode', '-nographics', '-crossOut', OUT, '-crossTag', tag,
                             '-crossListen', str(UNITY_PORTS[tag]), '-crossReply', str(TEXT_REPLY),
                             '-logFile', os.path.join(OUT, f'{tag}_player.txt')],
                            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def read_lines(path, side):
    lines = []
    if os.path.exists(path):
        for line in open(path, encoding='utf-8', errors='replace'):
            stamp, _, text = line.rstrip('\n').partition(' ')
            try:
                lines.append((float(stamp), side, text))
            except ValueError:
                pass
    return lines


# -- the bench ---------------------------------------------------------------

def unity_follows(tag, server, address, timeout=20):
    L.text(tag, 'follow', server, address)
    got = until(lambda: (lambda s: s if s and s[0] == 'Connected' and s[1] == '1' else None)(
        ask_text(tag, ['status'], 'status', 1)), timeout, 0.5)
    return got


def td_follows(how, args, timeout=20):
    L.text('td', 'follow', how, *args)
    return until(lambda: (lambda s: s if s and s[0] == '1' and s[1].startswith('connected') else None)(
        ask_text('td', ['status'], 'status', 1)), timeout, 0.5)


def max_connects(role, words, timeout=20):
    L.take_max('/cross/connected', role)
    L.max('/cross/connect', role, *words)
    return until(lambda: L.take_max('/cross/connected', role), timeout, 0.2)


def run_pairs(sample_patch):
    u1, u2 = Unity('unity1'), Unity('unity2')
    name, osc_port, ws = UNITY_SERVER
    url = f'ws://127.0.0.1:{ws}'
    address = '/probe/CrossFields/value'

    # Unity serves
    check('Unity serves unity_cross', bool(ask_text('unity1', ['serve', name, osc_port, ws], 'served', 10)))
    time.sleep(2)

    # Max follows Unity
    mc = Max('client')
    check('Max connects to unity_cross by address', bool(max_connects('client', ['oscquery', url])))
    pair_checks('Unity server x Max client (by address)', u1, mc, 0)
    L.max('/cross/disconnect', 'client')
    time.sleep(1)
    check('Max connects to unity_cross by name', bool(max_connects('client', [name])))
    both_ways('Unity server x Max client (by name)', u1, mc, 1)
    L.max('/cross/disconnect', 'client')
    time.sleep(1)

    # TouchDesigner follows Unity
    tc = TD('client')
    got = td_follows('addr', ['127.0.0.1', ws, address])
    check('TouchDesigner connects to unity_cross by address', bool(got), f'{got}')
    pair_checks('Unity server x TouchDesigner client (by address)', u1, tc, 2)
    L.text('td', 'unfollow')
    time.sleep(1)
    got = td_follows('name', [name, address])
    check('TouchDesigner connects to unity_cross by name', bool(got), f'{got}')
    both_ways('Unity server x TouchDesigner client (by name)', u1, tc, 3)
    L.text('td', 'unfollow')
    time.sleep(1)

    # A second Unity follows Unity
    got = unity_follows('unity2', url, address)
    check('Unity (unity2) connects to unity_cross by address', bool(got), f'{got}')
    pair_checks('Unity server x Unity client (by address)', u1, u2, 4)
    L.text('unity2', 'unfollow')
    time.sleep(1)
    got = unity_follows('unity2', name, address)
    check('Unity (unity2) connects to unity_cross by name', bool(got), f'{got}')
    both_ways('Unity server x Unity client (by name)', u1, u2, 5)
    L.text('unity2', 'unfollow')
    time.sleep(1)

    # Unity follows Max
    md = Max('device')
    got = ask_max('/cross/expose', [], '/cross/exposed', timeout=10)
    check('Max exposes max_cross', bool(got and got[0] == 1), f'{got}')
    dname, _, dws = MAX_DEVICE
    got = unity_follows('unity2', f'ws://127.0.0.1:{dws}', '/probe/value')
    check('Unity (unity2) connects to max_cross by address', bool(got), f'{got}')
    pair_checks('Max server x Unity client (by address)', md, u2, 6)
    L.text('unity2', 'unfollow')
    time.sleep(1)
    got = unity_follows('unity2', dname, '/probe/value')
    check('Unity (unity2) connects to max_cross by name', bool(got), f'{got}')
    both_ways('Max server x Unity client (by name)', md, u2, 7)
    L.text('unity2', 'unfollow')
    time.sleep(1)

    # Unity follows TouchDesigner
    ts = TD('server')
    tname, thttp, tosc = TD_SERVER
    check('TouchDesigner serves td_cross', bool(ask_text('td', ['serve', tname, thttp, tosc], 'served', 10)))
    time.sleep(2)
    got = unity_follows('unity2', f'ws://127.0.0.1:{thttp}', '/probe/value')
    check('Unity (unity2) connects to td_cross by address', bool(got), f'{got}')
    pair_checks('TouchDesigner server x Unity client (by address)', ts, u2, 8)
    L.text('unity2', 'unfollow')
    time.sleep(1)
    got = unity_follows('unity2', tname, '/probe/value')
    check('Unity (unity2) connects to td_cross by name', bool(got), f'{got}')
    both_ways('TouchDesigner server x Unity client (by name)', ts, u2, 9)
    L.text('unity2', 'unfollow')

    # The sample "Follow a device" and its patch
    fname, fws = FOLLOW
    for how, server in (('by name', fname), ('by address', f'ws://127.0.0.1:{fws}')):
        got = ask_text('unity1', ['scene', server], 'scened', 15)
        check(f'Follow a device ({how}): Unity loads the scene, its Client follows {server}', bool(got), f'{got}')
        cube = until(lambda: (lambda c: c if c and c[0] == 'Connected' else None)(
            ask_text('unity1', ['cube'], 'cube', 1)), 20, 0.5)
        check(f'Follow a device ({how}): the scene is connected', bool(cube), f'{cube}')
        if how == 'by name':
            ok = max_connects('observer', ['oscquery', f'ws://127.0.0.1:{fws}'])
            check('Follow a device: Max follows follow_me as another client', bool(ok))
        size = 1.5 + (how == 'by address')
        L.max('/cross/reset')
        time.sleep(0.2)
        L.max('/cross/put', 'observer', 'size', float(size))
        got = until(lambda: (lambda c: c if c and c[0] == 'Connected' and same(c[1], size) else None)(
            ask_text('unity1', ['cube'], 'cube', 1)), 5, 0.3)
        check(f'Follow a device ({how}): Max writes size {size:g}, the cube takes it', bool(got), f'{got}')
        a1 = ask_max('/cross/count', ['observer', 'angle'], '/cross/counted', 'observer')
        time.sleep(1)
        a2 = ask_max('/cross/count', ['observer', 'angle'], '/cross/counted', 'observer')
        check(f'Follow a device ({how}): the angle Unity sends reaches Max and moves',
              bool(a1 and a2 and a2[2] > a1[2] and a2[3] != a1[3]), f'{a1} then {a2}')
        back = ask_max('/cross/count', ['observer', 'size'], '/cross/counted', 'observer')
        check(f'Follow a device ({how}): nothing of what Max wrote comes back to it', bool(back and back[2] == 0),
              f'{back}')


def main():
    global OUT, L
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('folder', help='run folder, outside the repository')
    ap.add_argument('--package', required=True, help='the Unity package .tgz of the branch')
    ap.add_argument('--td-project', required=True, help='TouchDesigner project folder with the two .tox')
    ap.add_argument('--td-tests', required=True, help='oscquery/tests of prism-td-components (docs_run.py)')
    ap.add_argument('--unity-app', help='Unity.app (default: the newest 6000.3 of Unity Hub)')
    ap.add_argument('--td', help='TouchDesigner 2025 .app (default: the newest in /Applications)')
    ap.add_argument('--max', default='Max', help='Max application for open -a (default: Max)')
    a = ap.parse_args()
    folder = os.path.abspath(os.path.expanduser(a.folder))
    tgz = os.path.abspath(os.path.expanduser(a.package))
    td_project = os.path.abspath(os.path.expanduser(a.td_project))
    sys.path.insert(0, os.path.abspath(os.path.expanduser(a.td_tests)))
    import docs_run
    td_app = os.path.abspath(a.td) if a.td else docs_run.find_td()
    td_version = plistlib.load(open(os.path.join(td_app, 'Contents', 'Info.plist'), 'rb'))['CFBundleVersion']
    td_bin = os.path.join(td_app, 'Contents', 'MacOS', 'TouchDesigner')
    missing = [n for n in ('prism_oscquery.tox', 'prism_oscquery_server.tox', '.venv', 'TDPyEnvManagerContext.yaml')
               if not os.path.exists(os.path.join(td_project, n))]
    if missing:
        sys.exit(f'{td_project} lacks {", ".join(missing)}')
    if subprocess.run(['pgrep', '-f', td_bin], capture_output=True).stdout.strip():
        sys.exit(f'TouchDesigner {td_version} is already running: quit it first')

    os.makedirs(folder, exist_ok=True)
    player, sample = prepare_player(os.path.abspath(a.unity_app) if a.unity_app else find_unity(), folder, tgz)
    OUT = os.path.join(folder, 'cross_run')
    os.makedirs(OUT, exist_ok=True)
    logs = {'runner': 'runner.log', 'max': 'max.log', 'td': 'td.log', 'unity1': 'unity1.log', 'unity2': 'unity2.log'}
    logs = {side: os.path.join(OUT, name) for side, name in logs.items()}
    for path in logs.values():
        open(path, 'w').close()
    L = Links()
    print(f'Cross bench: package {os.path.basename(tgz)}, TouchDesigner {td_version}', flush=True)

    procs = []
    toe = os.path.join(td_project, 'cross_test.toe')
    try:
        if udp_taken(MAX_PORT):
            sys.exit(f'UDP {MAX_PORT} is taken: a cross.maxpat is still open in Max, close it first')
        subprocess.run(['open', '-a', a.max, os.path.join(HERE, 'cross.maxpat')], check=True)
        # One follow_me: a FollowADevice.maxpat already open in Max is used as it is
        if listening(FOLLOW[1]):
            log(f'INFO follow_me already answers on {FOLLOW[1]}: FollowADevice.maxpat not opened again')
        else:
            subprocess.run(['open', '-a', a.max, os.path.join(sample, 'FollowADevice.maxpat')], check=True)
        got = until(lambda: ask_max('/cross/hello', [logs['max']], '/cross/ready', timeout=1), 120, 1)
        check('Max: cross.maxpat answers', bool(got))
        docs_run.build_toe_text(td_app, toe, START_TEXT.format(script=os.path.join(HERE, 'cross_td.py'), out=OUT))
        procs.append(subprocess.Popen([td_bin, toe], cwd=td_project, stdout=open(os.path.join(OUT, 'td_stdout.txt'), 'w'),
                                      stderr=subprocess.STDOUT))
        procs += [launch(player, 'unity1'), launch(player, 'unity2')]
        for peer, timeout in (('td', 120), ('unity1', 30), ('unity2', 30)):
            got = until(lambda: ask_text(peer, ['hello'], 'ready', 1), timeout, 1)
            check(f'{peer} answers', bool(got))
        run_pairs(sample)
    except Exception as e:
        import traceback
        check('cross_run.py ran to its end', False, traceback.format_exc().replace('\n', ' | '))
    finally:
        for peer in ('unity1', 'unity2', 'td'):
            L.text(peer, 'quit')
        L.max('/cross/close')
        time.sleep(3)
        for p in procs:
            try:
                p.wait(20)
            except subprocess.TimeoutExpired:
                p.terminate()
        if os.path.exists(toe):
            os.remove(toe)

    merged = sorted(sum((read_lines(path, side) for side, path in logs.items()), []), key=lambda l: l[0])
    t0 = merged[0][0] if merged else time.time()
    report = [f'Cross bench, {time.strftime("%Y-%m-%d %H:%M")}, package {os.path.basename(tgz)}, '
              f'TouchDesigner {td_version}', '']
    report += [f'{stamp - t0:8.2f}  {side:8}  {text}' for stamp, side, text in merged]
    report += ['', 'Summary:'] + ['  ' + text[5:] for _, side, text in merged if side == 'runner' and text.startswith('INFO ')]
    fails = [text for _, side, text in merged if side == 'runner' and text.startswith('FAIL')]
    report += ['', f'{results["PASS"]} PASS, {results["FAIL"]} FAIL'] + ['  ' + t for t in fails]
    ok = not fails and results['PASS'] > 0
    report.append('RESULT: ' + ('PASS' if ok else 'FAIL'))
    text = '\n'.join(report) + '\n'
    open(os.path.join(OUT, 'report.txt'), 'w').write(text)
    print('\n' + text, end='')
    print(f'(written to {os.path.join(OUT, "report.txt")})')
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
