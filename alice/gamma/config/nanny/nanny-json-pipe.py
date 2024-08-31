#!/usr/bin/python -u
# -*- coding: utf-8 -*-

import json
import os
import sys


def main():
    while True:
        line = sys.stdin.readline()
        if line == '':
            break

        x, y, z, message = line.split(';', 3)  # piping with logrotate has format {x};{y};{z};{message}
        j = json.loads(message.strip())
        j['env'] = os.environ.get('GAMMA_ENV_LABEL', 'undefined')
        j['nanny_service_id'] = os.environ.get('NANNY_SERVICE_ID', 'undefined')
        message = json.dumps(j)

        print('{x};{y};{z};{message}'.format(x=x, y=y, z=z, message=message))


if __name__ == '__main__':
    main()
