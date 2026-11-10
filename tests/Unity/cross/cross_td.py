# SPDX-FileCopyrightText: 2026 CNRS
# SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>
"""The TouchDesigner peer of the cross bench, inside TouchDesigner.

Not run by hand: cross_run.py builds a project whose Execute DAT runs this
file when TouchDesigner starts, with CROSS_OUT (the run folder) set. It
uses the two components of the TouchDesigner project folder,
prism_oscquery_server.tox and prism_oscquery.tox, and answers the runner
over text datagrams on 127.0.0.1: it listens on 7494 and answers on 7498,
every reply prefixed "td".

Commands:
  hello                                   -> ready
  serve <name> <http> <osc>               COMP probe with Floats Value and Other, published -> served <status>
  follow addr <host> <port> <address>     prism_oscquery on ws://host:port
  follow name <name> <address>            prism_oscquery, Discover on, the device of that name
  status                                  -> status <bound 0|1> <Status>
  unfollow
  set <v>                                 the followed leaf, or Value when serving
  other <v>                               Other of the probe, which no client of the bench follows
  read                                    -> value <v>
  stats                                   -> stats <received> <sent> <echoes>
  quit
"""
import os
import socket
import time
import traceback

import td
from td import *  # noqa: F401,F403  op, project, run

OUT = CROSS_OUT  # noqa: F821  set by the Execute DAT that runs this file
LOG = os.path.join(OUT, 'td.log')
LISTEN, REPLY = 7494, 7498

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(('127.0.0.1', LISTEN))
sock.setblocking(False)
root = op('/project1')
state = {'server': None, 'probe': None, 'client': None, 'address': None, 'discover': None, 'quit': False}


def log(text):
    with open(LOG, 'a') as f:
        f.write('%.3f %s\n' % (time.time(), text))


def tell(text):
    sock.sendto(('td ' + text).encode(), ('127.0.0.1', REPLY))


def leaf():
    """The parameter the bench writes and reads."""
    c = state['client']
    if c is not None:
        names = {r[0].val: r[1].val for r in c.op('names').rows()[1:]}
        name = names.get(state['address'])
        return getattr(c.par, name, None) if name else None
    p = state['probe']
    return p.par.Value if p is not None else None


def command(words):
    log('command ' + ' '.join(words))
    w = words[0]
    if w == 'hello':
        tell('ready')
    elif w == 'serve':
        probe = root.create(baseCOMP, 'probe')
        page = probe.appendCustomPage('Probe')
        page.appendFloat('Value')
        page.appendFloat('Other')
        server = root.loadTox(project.folder + '/prism_oscquery_server.tox')
        server.name = 'cross_server'
        server.par.Target = probe
        server.par.Devicename = words[1]
        server.par.Httpport = int(words[2])
        server.par.Oscport = int(words[3])
        server.par.Announce = True
        server.par.Active = True
        state['server'], state['probe'] = server, probe
        tell('served')
    elif w == 'follow':
        c = root.loadTox(project.folder + '/prism_oscquery.tox')
        c.name = 'cross_client'
        state['client'] = c
        if words[1] == 'addr':
            c.par.Host = words[2]
            c.par.Port = int(words[3])
            state['address'] = words[4]
            c.par.Active = True
        else:
            c.par.Discover = True
            state['discover'] = words[2]
            state['address'] = words[3]
    elif w == 'status':
        c = state['client']
        if c is None:
            tell('status 0 none')
        else:
            tell(f'status {1 if leaf() is not None else 0} {c.par.Status.eval()}')
    elif w == 'unfollow':
        c = state['client']
        if c is not None:
            c.par.Active = False
            c.destroy()
        state['client'] = state['address'] = state['discover'] = None
    elif w == 'set':
        p = leaf()
        if p is not None:
            p.val = float(words[1])
    elif w == 'other':
        if state['probe'] is not None:
            state['probe'].par.Other = float(words[1])
    elif w == 'read':
        p = leaf()
        tell('value ' + ('-' if p is None else '%g' % p.eval()))
    elif w == 'stats':
        c = state['client']
        if c is not None:
            s = c.ext.OscqueryExt.session.stats
            tell(f"stats {s.get('received', 0)} {s.get('sent', 0)} {s.get('echoes', 0)}")
        elif state['server'] is not None:
            s = state['server'].ext.OscqueryServerExt.server.stats
            tell(f"stats {s.get('received', 0)} {s.get('sent', 0)} 0")
        else:
            tell('stats 0 0 0')
    elif w == 'quit':
        state['quit'] = True


def discover():
    """Picks the device announced under the name asked, once listed."""
    c, name = state['client'], state['discover']
    if c is None or not name:
        return
    for key, label in zip(c.par.Device.menuNames, c.par.Device.menuLabels):
        if label == name or label.startswith(name + ' ('):
            c.par.Device = key
            c.par.Active = True
            state['discover'] = None
            log(f'discovered {label}')
            return


def step():
    try:
        while True:
            try:
                data, _ = sock.recvfrom(65536)
            except (BlockingIOError, OSError):
                break
            words = data.decode().split()
            if words:
                command(words)
        discover()
    except Exception:
        log('FAIL cross_td.py: ' + traceback.format_exc().replace('\n', ' | '))
    if state['quit']:
        log('quit')
        sock.close()
        run(lambda: project.quit(force=True), delayFrames=10)
        return
    run(step, delayFrames=1)


open(LOG, 'a').close()
log(f'START TouchDesigner {app.build}, project {project.folder}')
run(step, delayFrames=30)
