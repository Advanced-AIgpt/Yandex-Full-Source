# coding:utf-8

import argparse
import json
import sys


content_type = 'Content-Type: application/json'


def main(args):
    i = 0
    head = 'POST /skill/%s HTTP/1.1' % args.skill_id
    host = 'HOST: %s' % args.host
    for line in sys.stdin:
        line = line.split('=')[1]
        request = {
            "meta": {
                "client_id": "ru.yandex.searchplugin/7.16 (none none; android 4.4.2)",
                "interfaces": {"account_linking": {}, "payments": {}, "screen": {}},
                "locale": "ru-RU",
                "timezone": "UTC"
            },
            "request": {
                "command": line,
                "nlu": {"entities": [], "tokens": []},
                "original_utterance": line,
                "type": "SimpleUtterance"
            },
            "session": {
                "message_id": (i / 50),
                "new": (i / 50 == 0),
                "session_id": "31337-7357-00000003-%06x" % i,
                "skill_id": "31337-7357-4fac-b40c-061914390741",
                "user_id": "31337%010x" % (i % 50)
            },
            "version": "1.0"
        }
        request_body = json.dumps(request)
        content_length = 'Content-Length: %s' % len(request_body)
        result = '\n'.join((head, content_length, content_type, host, '', request_body))
        sys.stdout.write('\n'.join((str(len(result)), result, '', '')))
        i += 1


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--skill', dest='skill_id', required=True)
    parser.add_argument('--host', dest='host', default='gamma-test.alice.yandex.net')

    main(parser.parse_args())
