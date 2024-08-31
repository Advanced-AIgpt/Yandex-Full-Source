#!/usr/bin/env python
"""
Pipe filter for push-client
"""
import json
import os
import sys
import errno
import time
TZ = time.altzone if time.daylight and time.localtime().tm_isdst > 0 else time.timezone
TZ_FORMATED = '{}{:0>2}{:0>2}'.format('-' if TZ > 0 else '+', abs(TZ) // 3600, abs(TZ // 60) % 60)
ATTRS_FILE = "dump.json"
HOSTNAME = os.uname()[1]
SKELETON = {
    "itype": "unknown",
    "ctype": "stable",
    "prj": "unknown",
    "metaprj": "voicetech"
}
UNIPROXY = {
    "itype": "uniproxy",
    "ctype": "prod",
    "prj": "uniproxy",
    "metaprj": "unknown"
}


def get_value_from_gencfg_tags(tag_string):
    """
    Filter tags from dump.json
    """
    properties_tags = tag_string.split(" ")
    result = SKELETON
    for raw_tag in properties_tags:
        tag = raw_tag.split('_')
        if tag[0] == "a":
            result[tag[1]] = tag[2]
    return result


def read_tags():
    """
    Read dump.json for tags
    """
    try:
        with open(ATTRS_FILE) as dumpjson:
            dump = json.load(dumpjson)
        tags = get_value_from_gencfg_tags(dump['properties']['tags'])
    except IOError:
        # sys.exit("Config file not found %s" % ATTRS_FILE)
        tags = UNIPROXY
    except KeyError:
        # sys.exit("Parameter not found in %s" % ATTRS_FILE)
        tags = UNIPROXY
    return tags


def main():
    """
    Main function
    """
    tags = read_tags()
    try:
        for line in sys.stdin:
            if line == '':
                continue
            try:
                secret, row_id, offset, message = line.split(";", 3)
            except ValueError:
                continue
            log_data = {
                'pushclient_row_id': row_id,
                'level': 20000,
                'levelStr': 'INFO',
                'loggerName': 'stdout',
                '@version': 1,
                'threadName': 'qloud-init',
                '@timestamp': time.strftime("%Y-%m-%dT%H:%M:%S", time.localtime()) + TZ_FORMATED,
                'qloud_project': tags['metaprj'],
                'qloud_application': tags['itype'],
                'qloud_environment': tags['ctype'],
                'qloud_component': tags['prj'],
                'qloud_instance': '-',
                'message': message.strip('\n'),
                'host': HOSTNAME
            }
            line = "{};{};{};{}".format(secret, row_id, offset, json.dumps(log_data)).strip()
            print(line)
            sys.stdout.flush()
    except IOError as err:
        if err.errno == errno.EPIPE:
            sys.exit(0)
        else:
            sys.exit(1)


if __name__ == '__main__':
    main()
