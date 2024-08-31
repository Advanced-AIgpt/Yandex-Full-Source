#! /usr/bin/env python2
import argparse
import subprocess
import datetime
import os
import sys

dir = 'norm'

keyval = {
    'lang': None,
    'model': 'automotive',
    'case': 'lower',
    'split': 'no',
    'date': None,
    'version': None,
    'maintainer': 'gogabr@yandex-team.ru',
    'tooling-ver': None,
    'norm-data-format': 'fst',
}

volatile_keys = {'date', 'version', 'maintainer', 'tooling-ver'}

lang_short_to_full = {
    'en': 'en-US',
    'de': 'de-DE',
}

parser = argparse.ArgumentParser(description='Generate a MANIFEST file')
parser.add_argument('--reverse', action='store_const', const=True)
parser.add_argument('argpairs', metavar='key=value', nargs='*')
args = parser.parse_args()

print args

if args.reverse:
    dir='revnorm'

for p in args.argpairs:
    [k, v] = p.split('=')
    keyval[k] = v

if keyval['lang'] is None:
    keyval['lang'] = lang_short_to_full[os.path.basename(os.getcwd())]

if keyval['date'] is None:
    keyval['date'] = datetime.date.today().strftime('%y%m%d')

if keyval['tooling-ver'] is None:
    revision = subprocess.check_output(['git', 'rev-parse', 'HEAD']).strip()
    keyval['tooling-ver'] = revision

#print keyval

check_failed = False
for k in keyval:
    if keyval[k] is None:
        print >>sys.stderr, 'Need value for %s' % k
        check_failed = True
if check_failed:
    exit(1)

with open(os.path.join(dir, 'MANIFEST'), 'w') as of:
    for k in keyval:
        prefix=''
        if k in volatile_keys:
            prefix='$'
        print >>of, '%s%s=%s' % (prefix, k, keyval[k])
