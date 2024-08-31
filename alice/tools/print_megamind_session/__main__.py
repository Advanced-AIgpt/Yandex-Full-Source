# coding:utf-8

import argparse
import base64
import json
import zlib
import sys

from google.protobuf import text_format

from alice.megamind.library.session.protos.session_pb2 import TSessionProto
from alice.hollywood.library.scenarios.music.proto.music_context_pb2 import TScenarioState  # noqa
# Feel free to import protos from your scenarios here...


def decompress(session):
    return zlib.decompress(base64.decodebytes(session.encode('ascii')))


def decompress_megamind(session):
    return base64.decodebytes(session.encode('ascii'))


def print_session(session, scenario=None):
    vins_session = json.loads(decompress(session))
    megamind_session = TSessionProto()
    if '__megamind__' in vins_session:
        megamind_session.ParseFromString(decompress_megamind(vins_session['__megamind__']))
        del vins_session['__megamind__']
    if scenario:
        print('-' * 32 + f'{scenario} State' + '-' * 32)
        print(text_format.MessageToString(megamind_session.ScenarioSessions[scenario]))
    else:
        print('-' * 34 + 'Vins Session' + '-' * 34)
        print('{}'.format(json.dumps(vins_session, ensure_ascii=False, indent=2)))
        print('-' * 32 + 'Megamind Session' + '-' * 32)
        print(text_format.MessageToString(megamind_session))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--scenario", type=str, help='Name of scenario which State you want to print')
    args = parser.parse_args()
    print_session(session=sys.stdin.read(), scenario=args.scenario)


if __name__ == '__main__':
    main()
