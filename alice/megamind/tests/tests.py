#!/usr/bin/env python2

import os
import os.path
import subprocess
import sys

MYDIR = os.path.realpath(os.path.dirname(__file__))
sys.path.append(os.path.join(MYDIR, 'library'))

import ya_tool

if __name__ == '__main__':
    for idx, val in enumerate(sys.argv):
        if val in ['tests', 'info', 'push']:
            run_dir = os.path.join(MYDIR, 'run')
            cmd = [ya_tool.ya_tool(), 'make'] + sys.argv[1:idx] + [run_dir]
            result = subprocess.call(cmd)
            if result:
                sys.exit(result)

            cmd = [os.path.join(run_dir, 'run')] + sys.argv[idx:] + ['--'] + sys.argv[1:idx]
            os.execv(cmd[0], cmd)

    sys.stderr.write('Unkown command\n')
    sys.exit(1)
