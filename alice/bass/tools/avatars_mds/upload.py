#!/usr/bin/env python
# -*- coding: utf-8 -*-

import json
import sys
import zipfile
import tempfile
import requests
import os
import hashlib
import time
import imghdr
import struct
import argparse

from view_icons import generate_html

profile = {
        'prod': {
            'wurl': 'http://avatars-int.mds.yandex.net:13000/put-bass/{}',
            'rurl': '://avatars.mds.yandex.net'
        },
        'test': {
            'wurl': 'http://avatars-int.mdst.yandex.net:13000/put-bass/{}',
            'rurl': '://avatars.mdst.yandex.net'
        }
}

class Images:
    def __init__(self, args):
        self.upload_type = args.namespace
        self.profile = profile[args.profile]
        self.files = []
        tempdir = tempfile.mkdtemp('_bass_icons_avatars')
        with zipfile.ZipFile(args.archive, 'r') as zipref:
            for f in zipref.infolist():
                name, date_time = f.filename, f.date_time
                name = os.path.join(tempdir, name)
                with open(name, 'wb') as f_dst:
                    f_dst.write(zipref.open(f).read())
                date_time = time.mktime(date_time + (0, 0, -1))
                os.utime(name, (date_time, date_time))

        for fn in os.listdir(tempdir):
            filename = os.path.join(tempdir, fn)
            width, height = Images.get_size(filename)
            mtime = os.stat(filename).st_mtime
            name = '{}_{}x{}_'.format(self.upload_type, width, height) + hashlib.sha256("{}.{}".format(mtime, fn)).hexdigest() + '.png'
            if args.pretend:
                sys.stderr.write("{} => {}\n".format(fn, name))
            else:
                response = self.fetch(name, open(filename, 'r'))
                js = response.json()
                description = js.get('description')
                status = js.get('status')
                if 'attrs' in js:
                    js = js['attrs']

                if args.verbose > 0:
                    sys.stderr.write("[{}] {} => {} ({})\n".format(status, fn, name, description))
                    if args.verbose > 1:
                        sys.stderr.write('  https{}{}\n'.format(self.profile['rurl'], js['sizes']['orig']['path']))
                        if args.verbose > 2:
                            sys.stderr.write('  {}\n\n'.format(response.text))

                self.files.append({ "json": js, "id": fn })
                try:
                    js['sizes'] # just for check!!!
                except Exception as e:
                    sys.stderr.write(" RESPONSE: {}\n".format(response))
                    sys.stderr.write(" RESPONSE TEXT: {}\n".format(response.text))
                    raise e

    def generate(self, output_json, args):
        if not args.keepns or self.upload_type not in output_json:
            output_json[self.upload_type] = {}

        for item in self.files:
            try:
                u = self.profile['rurl'] + item['json']['sizes']['orig']['path']
                output_json[self.upload_type][item['id']] = {
                        'http': 'http' + u,
                        'https': 'https' + u
                }
            except Exception as e:
                print(json.dumps(item, sort_keys=True, indent=4))
                pass
        json.dump(output_json, open(args.output_json, 'w'))

    def fetch(self, name, fd):
        data = fd.read()
        url = self.profile['wurl'].format(name)
        headers = {
                'Content-Disposition': 'filename="file.jpeg"',
                'Content-Length': str(len(data))
        }
        files = { 'file': data }
        for i in range(5):
            try:
                answer = requests.post(url, files=files, headers=headers)
                return answer
            except Exception as ex:
                print >> sys.stderr, str(ex)
                continue
        return None

    @staticmethod
    def get_size(fname):
        with open(fname, 'rb') as fhandle:
            head = fhandle.read(24)
            if len(head) != 24:
                return
            if imghdr.what(fname) == 'png':
                check = struct.unpack('>i', head[4:8])[0]
                if check != 0x0d0a1a0a:
                    return
                width, height = struct.unpack('>ii', head[16:24])
            elif imghdr.what(fname) == 'gif':
                width, height = struct.unpack('<HH', head[6:10])
            elif imghdr.what(fname) == 'jpeg':
                try:
                    fhandle.seek(0) # Read 0xff next
                    size = 2
                    ftype = 0
                    while not 0xc0 <= ftype <= 0xcf:
                        fhandle.seek(size, 1)
                        byte = fhandle.read(1)
                        while ord(byte) == 0xff:
                            byte = fhandle.read(1)
                        ftype = ord(byte)
                        size = struct.unpack('>H', fhandle.read(2))[0] - 2
                    # We are at a SOFn block
                    fhandle.seek(1, 1)  # Skip `precision' byte.
                    height, width = struct.unpack('>HH', fhandle.read(4))
                except Exception: #IGNORE:W0703
                    return
            else:
                return
            return width, height

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--pretend', '-p', action='store_true', help='do not upload to avatarica')
    parser.add_argument('--profile', choices=['prod', 'test'], default='test', help='upload profile')
    parser.add_argument('--verbose', '-v', action='count', help='verbosity level')
    parser.add_argument('--keepns', action='store_true', help='do not clean given namespace (by default it cleans)')
    parser.add_argument('namespace', help='namespace (weather, games, etc...)')
    parser.add_argument('archive', help='zip archive with icons (all icons must be in the root')
    parser.add_argument('output_json', help='output json file for ya.make (if file exists - will be merged)')
    args = parser.parse_args()

    images=Images(args)
    if not args.pretend:
        output_json = {}
        try:
            with open(args.output_json, 'r') as f:
                output_json = json.load(f)
        except:
            pass

        images.generate(output_json, args)
        generate_html(output_json, args.output_json + '.html')

if __name__ == '__main__':
    main()
