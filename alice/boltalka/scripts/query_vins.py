#!/usr/bin/python
import argparse
import re
import pytz
import sys
import calendar
import codecs
import requests
import simplejson as json
sys.stdin = codecs.getreader('utf-8')(sys.stdin)
sys.stdout = codecs.getwriter('utf-8')(sys.stdout)

from uuid import uuid1 as gen_uuid
from datetime import datetime
from requests.exceptions import RequestException

class SpeechKitAPI:
    TIMEOUT = 5.0

    def __init__(self, vins_url, bass_url=None):
        self._vins_url = vins_url
        self._bass_url = bass_url

    def say(self, uuid, dt, utterance, location=None):
        request_id = str(gen_uuid())

        request = {
            'header': {
                'request_id': request_id
            },
            'application': {
                'app_id': 'com.yandex.vins.tests',
                'app_version': '0.0.1',
                'os_version': '1',
                'platform': 'unknown',
                'uuid': uuid,
                'lang': 'ru-RU',
                'client_time': dt.strftime('%Y%m%dT%H%M%S'),
                'timezone': dt.tzinfo.zone,
                'timestamp': str(calendar.timegm(dt.timetuple()))
            },
            'request': {
                'additional_options': {
                    'bass_url': self._bass_url
                },
                'event': {
                    'type': 'text_input',
                    'text': utterance
                },
                'location': location,
                'experiments': ['general_conversation']
            }
        }
        headers = {'Content-type': 'application/json'}
        response = requests.post(self._vins_url, data=json.dumps(request), timeout=self.TIMEOUT, headers=headers, verify=False)
        response.raise_for_status()
        return response.json()['response']['cards'][0]['text']

LOCATION = {
    'lon': 37.587937,
    'lat': 55.733771
}  # Yandex Office

def process_query(speechkit_client, context):
    uuid = str(gen_uuid())
    tz = pytz.timezone('Europe/Moscow')

    dialog = []
    for i, utterance in enumerate(context):
        if i % 2 != 0:
            continue
        dialog.append(utterance)
        dt = datetime.now(tz)

        try:
            response = speechkit_client.say(uuid, dt, utterance, LOCATION)
        except RequestException as e:
            print 'FAIL', context
            sys.exit(1)

        response = '\\n'.join(response.split('\n'))
        response = '\\t'.join(response.split('\t'))
        dialog.append(response)

    return dialog

def parse_yt_input(line):
    dct = {}
    for part in line.rstrip().split('\t'):
        if not '=' in part:
            continue
        k, v = part.split('=', 1)
        dct[k] = v
    return dct

def get_context(dct):
    turns = []
    for k, v in dct.iteritems():
        if k.startswith('context_'):
            i = int(k.split('_', 1)[1])
            turns.append((i, v))
    return [v for k, v in sorted(turns, reverse=True)]

def update_dict(dct, dialog):
    dct['nlg'] = dialog[-1]
    dialog.pop()
    for i, turn in enumerate(dialog):
        k = 'context_' + str(len(dialog) - i - 1)
        dct[k] = turn

def main():
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--vins-url', metavar='URL', dest='vins_url', required=False,
                        default='https://vins-int.tst.voicetech.yandex.net/speechkit/app/pa/',
                        help='A speechkit API url to test')
    parser.add_argument('--bass-url', metavar='URL', dest='bass_url', required=False,
                        help='Allows to overwrite BASS url')
    args = parser.parse_args()

    speechkit_client = SpeechKitAPI(args.vins_url, args.bass_url)

    successful = 0
    total = 0
    for i, line in enumerate(sys.stdin):
        dct = parse_yt_input(line)
        dialog = process_query(speechkit_client, get_context(dct))
        update_dict(dct, dialog)
        print '\t'.join(k + '=' + v for k, v in sorted(dct.iteritems()))
        i += 1
        print >> sys.stderr, i, 'lines done.\r',
    print >> sys.stderr

if __name__ == '__main__':
    main()
