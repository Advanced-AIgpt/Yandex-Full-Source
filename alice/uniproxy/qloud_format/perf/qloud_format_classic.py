#!/usr/bin/env python

import sys
import time
import socket
import json

hostname = socket.getfqdn()
tz = time.altzone if time.daylight and time.localtime().tm_isdst > 0 else time.timezone
tz_formatted = '{}{:0>2}{:0>2}'.format('-' if tz > 0 else '+', abs(tz) // 3600, abs(tz // 60) % 60)


def main():
    while True:
        try:
            line = sys.stdin.readline()
            if line == '':
                break
            secret, row_id, offset, message = line.split(";", 3)
            log_data = {
                'pushclient_row_id': row_id,
                'level': 20000,
                'levelStr': 'INFO',
                'loggerName': 'stdout',
                '@version': 1,
                'threadName': 'qloud-init',
                '@timestamp': time.strftime("%Y-%m-%dT%H:%M:%S", time.localtime()) + tz_formatted,
                'qloud_project': 'alice',
                'qloud_application': 'uniproxy',
                'qloud_environment': 'stable',
                'qloud_component': 'uniproxy',
                'qloud_instance': '-',
                'message': message,
                'host': hostname
            }
            line = "{};{};{};{}".format(secret, row_id, offset, json.dumps(log_data, separators=(',', ':'))).strip()
            print(line)
        except:
            pass


if __name__ == '__main__':
    main()
