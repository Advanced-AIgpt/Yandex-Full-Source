# -*- coding: utf-8 -*-

import base64
import json
import logging
import os
import requests
import shutil
import uuid
import zlib

from collections import deque

from google.protobuf import text_format
from google.protobuf.json_format import MessageToJson

from alice.acceptance.modules.request_generator.lib import vins
from alice.acceptance.modules.request_generator.lib import app_presets
from alice.megamind.library.session.protos.session_pb2 import TSessionProto
from alice.megamind.protos.scenarios.request_pb2 import TScenarioRunRequest

logger = logging.getLogger(__name__)


_CLIENT_TIME = '20200120T054430'

_MM_URL = 'http://nikola10.search.yandex.net:7007/speechkit/app/pa/'

_MM_REQUESTS_DIR = 'mm_requests'
_HW_REQUESTS_DIR = 'requests'


def _parse_session(session):
    session = json.loads(zlib.decompress(base64.decodebytes(session.encode('ascii'))))

    megamind_session = TSessionProto()
    if '__megamind__' in session:
        session = base64.decodebytes(session['__megamind__'].encode('ascii'))
        megamind_session.ParseFromString(session)

    return megamind_session


def _get_run_request(request_id):
    hw_request_path = os.path.join(_HW_REQUESTS_DIR, 'movie_akinator@run@{}'.format(request_id))

    if not os.path.isfile(hw_request_path):
        return None

    with open(hw_request_path) as f:
        run_request_text = f.read()

    run_request_proto = TScenarioRunRequest()
    text_format.Merge(run_request_text, run_request_proto)

    run_request = json.loads(MessageToJson(run_request_proto))
    run_request['base_request'] = {'state': run_request['base_request']['state']}
    run_request['input'] = {'semantic_frames': run_request['input']['semantic_frames']}

    return json.dumps(run_request, ensure_ascii=False, sort_keys=True)


def _request_mm(text, session, visited):
    logger.info(text)

    request_id = str(uuid.uuid4())

    request = vins.make_vins_request(
        request_id,
        text=text,
        event=None,
        lang='ru-RU',
        app=app_presets.SEARCH_APP_PROD.application,
        experiments=['mm_enable_protocol_scenario=MovieAkinator'],
        uuid='deadbeef-ffff-ffff-ffff-deadbeef1234',
        client_time=_CLIENT_TIME,
        session=session
    )
    request = json.dumps(request)

    requests_session = requests.Session()
    requests_session.headers.update({'Content-Type': 'application/json'})
    response = requests_session.post(_MM_URL, data=request).json()

    run_request = _get_run_request(request_id)

    if not run_request:
        logger.warning('Request %s was not processed by the scenario!', text)
        return

    if run_request in visited:
        logger.info('Visited request: %s', run_request)
        return

    visited.add(run_request)

    with open(os.path.join(_MM_REQUESTS_DIR, 'request@{}.json'.format(request_id)), 'w') as f:
        f.write(request)

    session = response['sessions'][""]
    parsed_session = _parse_session(session)

    for action_key, action in parsed_session.Actions.items():
        for hint in action.NluHint.Instances:
            yield hint.Phrase, session


def _prepare_requests():
    if os.path.isdir(_MM_REQUESTS_DIR):
        shutil.rmtree(_MM_REQUESTS_DIR)

    if os.path.isdir(_HW_REQUESTS_DIR):
        shutil.rmtree(_HW_REQUESTS_DIR)

    os.makedirs(_MM_REQUESTS_DIR)
    os.makedirs(_HW_REQUESTS_DIR)

    queue = deque([('посоветуй фильм', None)])
    visited = set()

    while queue:
        text, session = queue.popleft()

        for next_text, next_session in _request_mm(text, session, visited):
            queue.append((next_text, next_session))

    logger.info('Visited: %s', len(visited))


def _generate_ammo(path):
    run_request_proto = TScenarioRunRequest()
    with open(os.path.join(_HW_REQUESTS_DIR, path)) as f:
        text_format.Merge(f.read(), run_request_proto)

    body = text_format.MessageToString(
        run_request_proto,
        as_utf8=True,
        as_one_line=True
    )
    body = body.strip()

    request_body = 'POST /movie_akinator/run HTTP/1.1\n'
    request_body += 'Content-Length: {}\n'.format(len(body.encode('utf-8')))
    request_body += 'Content-Type: text/protobuf\n'
    request_body += 'Host: localhost\n\n'
    request_body += body

    return '{}\n{}\n'.format(len(request_body.encode('utf-8')), request_body)


def _generate_ammos():
    with open('ammo.txt', 'w') as f_out:
        for path in os.listdir(_HW_REQUESTS_DIR):
            ammo = _generate_ammo(path)
            f_out.write(ammo + '\n\n')


def main():
    logging.basicConfig(level=logging.INFO, format='[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s')

    _generate_ammos()


if __name__ == '__main__':
    main()
