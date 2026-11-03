#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 CNRS
# SPDX-FileContributor: Julien Bayle, PRISM <https://www.prism.cnrs.fr/>
"""The "doc only" test of the Unity sample "Follow a device", no click.

usage: python3 doc_follow.py <folder> --package <io.ossia.ossia-unity-*.tgz>
                             [--unity-app <Unity.app>] [--max <Max.app>]

What the README of the Unity package says, in a new project, outside the
repository: <folder>/doc_follow/.
1. A new Unity project; the package added as Install package from tarball
   adds it (a file: dependency on the .tgz in Packages/manifest.json).
2. The sample imported as the Samples tab imports it: Samples~/FollowADevice
   of the package copied to Assets/Samples/ossia (PRISM)/<version>/Follow a
   device.
3. In Max, FollowADevice.maxpat of that folder opened; cross.maxpat opened
   too, whose observer plays the hand that changes size and spin.
4. Unity in batch mode runs DocFollow.cs: opens the scene, presses Play,
   checks the console line and the Inspector of OssiaClient, size and spin
   set in Max reach the cube, the angle reaches Max.
The two steps of the Package Manager are made by their effect on the files,
not by its window. Prints RESULT: PASS or RESULT: FAIL.
"""
import argparse
import glob
import json
import os
import re
import shutil
import socket
import struct
import subprocess
import sys
import tarfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
SAMPLE = 'package/Samples~/FollowADevice/'


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('folder', help='where the project goes, outside the repository')
    ap.add_argument('--package', required=True, help='the Unity package .tgz')
    ap.add_argument('--unity-app', help='Unity.app (default: the newest 6000.3 of Unity Hub)')
    ap.add_argument('--max', default='Max', help='Max application for open -a (default: Max)')
    a = ap.parse_args()
    tgz = os.path.abspath(os.path.expanduser(a.package))
    found = sorted(glob.glob('/Applications/Unity/Hub/Editor/6000.3.*/Unity.app'))
    app = os.path.abspath(a.unity_app) if a.unity_app else (found[-1] if found else sys.exit('no Unity 6000.3'))
    unity = os.path.join(app, 'Contents', 'MacOS', 'Unity')
    folder = os.path.join(os.path.abspath(os.path.expanduser(a.folder)), 'doc_follow')
    project = os.path.join(folder, 'project')
    if os.path.exists(folder):
        shutil.rmtree(folder)
    os.makedirs(folder)

    print(f'1. new project {project}', flush=True)
    subprocess.run([unity, '-batchmode', '-quit', '-createProject', project, '-logFile',
                    os.path.join(folder, 'create.log')], stdin=subprocess.DEVNULL, timeout=900, check=True)
    manifest = os.path.join(project, 'Packages', 'manifest.json')
    m = json.load(open(manifest))
    m['dependencies']['io.ossia.ossia-unity'] = 'file:' + tgz
    m['dependencies'].setdefault('com.unity.test-framework', '1.6.0')
    json.dump(m, open(manifest, 'w'), indent=2)

    with tarfile.open(tgz) as t:
        version = json.load(t.extractfile('package/package.json'))['version']
        sample = os.path.join(project, 'Assets', 'Samples', 'ossia (PRISM)', version, 'Follow a device')
        print(f'2. the sample imported into {sample}', flush=True)
        os.makedirs(sample)
        for member in t.getmembers():
            if member.isfile() and member.name.startswith(SAMPLE) and '/' not in member.name[len(SAMPLE):]:
                with open(os.path.join(sample, member.name[len(SAMPLE):]), 'wb') as f:
                    f.write(t.extractfile(member).read())
    tests = os.path.join(project, 'Assets', 'DocTest')
    shutil.copytree(os.path.join(HERE, 'unity', 'DocTest'), tests)

    print('3. FollowADevice.maxpat and cross.maxpat opened in Max', flush=True)
    probe = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    probe.settimeout(0.5)
    if probe.connect_ex(('127.0.0.1', 9678)) == 0:
        sys.exit('follow_me already answers on 9678: close the FollowADevice.maxpat open in Max first')
    probe.close()
    subprocess.run(['open', '-a', a.max, os.path.join(sample, 'FollowADevice.maxpat')], check=True)
    subprocess.run(['open', '-a', a.max, os.path.join(HERE, 'cross.maxpat')], check=True)
    time.sleep(8)

    print('4. the scene played in Unity, batch mode', flush=True)
    log = os.path.join(folder, 'unity.log')
    results = os.path.join(folder, 'results.xml')
    env = dict(os.environ, DOC_FOLLOW_SCENE=os.path.relpath(os.path.join(sample, 'FollowADevice.unity'), project),
               DOC_FOLLOW_LOG=log)
    subprocess.run([unity, '-batchmode', '-projectPath', project, '-runTests', '-testPlatform', 'EditMode',
                    '-testFilter', 'DocFollow', '-testResults', results, '-logFile', log],
                   stdin=subprocess.DEVNULL, timeout=900, env=env)
    text = open(log, errors='replace').read() if os.path.exists(log) else ''
    for line in text.splitlines():
        if line.startswith('DOC ') or line.startswith('OSSIA client:'):
            print('   ' + line)
    xml = open(results).read() if os.path.exists(results) else ''
    passed = re.search(r'<test-case [^>]*name="PlayTheSceneWithThePatchOpen"[^>]*result="Passed"', xml) is not None
    if not passed:
        for msg in re.findall(r'<message><!\[CDATA\[(.*?)\]\]>', xml, re.S)[:2]:
            print('   ' + msg.strip()[:400])
    # cross.maxpat closes itself; FollowADevice.maxpat stays open in Max
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.sendto(b'/cross/close\0\0\0\0,\0\0\0', ('127.0.0.1', 7493))
    s.close()
    print('RESULT: ' + ('PASS' if passed else 'FAIL'))
    return 0 if passed else 1


if __name__ == '__main__':
    sys.exit(main())
