#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# web socket client for acceptance testing uniproxy using misc scenarious
#
import argparse
import datetime
import json
import logging
import os
import random
import struct
import sys
import time
import uuid

from copy import deepcopy
from multiprocessing import Pool
from tornado.websocket import websocket_connect
from tornado.ioloop import IOLoop
from tornado import gen
from uniclient import TestSession

logger = logging.getLogger(__name__)

UNIPROXY_URL = ''
AUTH_TOKEN = ''


def get_action_by_ref(script, action_ref):
    for action in script:
        aref = action.get('action_ref')
        if aref is not None and aref == action_ref:
            return action
    return None


def ensure_positive(val):
    return val if val >= 0 else 0


class AsrRecognizeTestSession(TestSession):
    def __init__(self, conn, script, auth_token, script_folder):
        TestSession.__init__(self, conn, script, auth_token, script_folder)
        self.start_session = time.time()
        self.recv_asr_result_time = None
        self.duration_asr_response = None  # diff asr_result - start_session

    def on_recv_message(self, rec, message):
        aref = rec.get('action_ref')
        if aref is not None:
            if aref == 'asr_result':
                self.recv_asr_result_time = time.time()
                self.duration_asr_response = ensure_positive(self.recv_asr_result_time - self.start_session)

    def store_timings(self, timings):
        timings['start_session'] = self.start_session
        if self.duration_asr_response is not None:
            timings['asr_result'] = self.duration_asr_response


class VinsTestSession(TestSession):
    def __init__(self, conn, script, auth_token, script_folder):
        TestSession.__init__(self, conn, script, auth_token, script_folder)
        self.start_session = time.time()


class VinsVoiceInputTestSession(VinsTestSession):
    def __init__(self, conn, script, auth_token, script_folder):
        VinsTestSession.__init__(self, conn, script, auth_token, script_folder)
        self.recv_asr_result_time = None
        self.duration_asr_response = None  # diff asr_result - start_session
        self.duration_vins_response = None  # diff vins_response - asr_result
        self.duration_total_vins_response = None  # diff vins_response - start_session

    def on_start_action(self, rec):
        aref = rec.get('action_ref')
        if aref is not None and aref == 'vins_response':
            self.start_vins_response_stage = time.time()

    def on_recv_message(self, rec, message):
        aref = rec.get('action_ref')
        if aref is not None:
            if aref == 'asr_result':
                self.recv_asr_result_time = time.time()
                self.duration_asr_response = ensure_positive(self.recv_asr_result_time - self.start_session)
            elif aref == 'vins_response':
                self.duration_vins_response = ensure_positive(time.time() - self.recv_asr_result_time)
                self.duration_total_vins_response = ensure_positive(time.time() - self.start_session)

    def store_timings(self, timings):
        timings['start_session'] = self.start_session
        if self.duration_asr_response is not None:
            timings['asr_result'] = self.duration_asr_response
        if self.duration_vins_response is not None:
            timings['vins_response'] = self.duration_vins_response
        if self.duration_total_vins_response is not None:
            timings['total_vins_response'] = self.duration_total_vins_response


class VinsTextInputTestSession(VinsTestSession):
    def __init__(self, conn, script, auth_token, script_folder):
        VinsTestSession.__init__(self, conn, script, auth_token, script_folder)
        self.recv_asr_result_time = None
        self.duration_vins_response = None  # diff vins_response - asr_result

    def on_start_action(self, rec):
        aref = rec.get('action_ref')
        if aref is not None and aref == 'vins_response':
            self.start_vins_response_stage = time.time()

    def on_recv_message(self, rec, message):
        aref = rec.get('action_ref')
        if aref is not None:
            if aref == 'vins_response':
                self.duration_vins_response = ensure_positive(time.time() - self.start_session)

    def store_timings(self, timings):
        timings['start_session'] = self.start_session
        if self.duration_vins_response is not None:
            timings['vins_response'] = self.duration_vins_response


def pool_func(args):
    sess_start = time.time()
    uniproxy_ws_url = UNIPROXY_URL
    auth_token = AUTH_TOKEN
    num, param, test_script = args
    test_script = deepcopy(test_script)
    audio_action = get_action_by_ref(test_script, 'send_audio_file')
    timelimit = get_action_by_ref(test_script, 'timelimit')
    if audio_action:
        audio_action['filename'] = param
        filesize = os.path.getsize(param)
        approximate_audio_duration = filesize / 10000.
        timelimit['timelimit'] = approximate_audio_duration + 30
        action = get_action_by_ref(test_script, 'asr_request')
        if action is not None:
            session_creator = AsrRecognizeTestSession
        else:
            session_creator = VinsVoiceInputTestSession
    else:
        action = get_action_by_ref(test_script, 'vins_request')
        action['message']['event']['payload']['request']['event']['text'] = param
        session_creator = VinsTextInputTestSession
    errors = []
    timings = {
        'start_test': sess_start
    }
    @gen.coroutine
    def ioloop_main():
        logger.info("run job={} param={}".format(num, param))
        conn = None
        session = None
        try:
            logger.debug('connect to uniproxy={}'.format(uniproxy_ws_url))
            conn = yield websocket_connect(uniproxy_ws_url)
            session = session_creator(conn, test_script, auth_token, os.path.dirname(__file__))
            yield session.process()
        except Exception as e:
            if session and session.error:
                errors.append(session.error)  # ignore exceptions caused force closing web socket (on timeout)
            else:
                errors.append(str(e))
            logger.exception('ioloop_main failed')
        if conn:
            conn.close()
        IOLoop.current().stop()
        if session:
            session.store_timings(timings)
    IOLoop.current().call_later(0, ioloop_main)
    IOLoop.current().start()
    IOLoop.clear_current()
    if not errors:
        logger.info('job={} ok'.format(num))
    else:
        logger.error('job={} failed:\n{}'.format(num, '\n'.join(errors)))
    timings['test_duration'] = ensure_positive(time.time() - sess_start)
    session_result = {
        'num': num,
        'param': os.path.basename(param),
        'errors': errors,
    }
    session_result.update(timings)
    return session_result


def main():
    root_logger = logging.getLogger('')
    file_logger = logging.StreamHandler()
    file_logger.setFormatter(logging.Formatter('%(asctime)s %(levelname)-8s %(filename)s:%(lineno)d %(message)s'))
    root_logger.addHandler(file_logger)
    root_logger.setLevel(logging.INFO)

    parser = argparse.ArgumentParser()

    parser.add_argument('--uniproxy', metavar='URL', default='wss://uniproxy.tst.voicetech.yandex.net/uni.ws',
                        help='url for access to tested uniproxy (wss://uniproxy.tst.voicetech.yandex.net/uni.ws)')
    parser.add_argument('--test-session', metavar='FILENAME',
                        help='read test session description from given filename')
    parser.add_argument('--auth-token', metavar='URL', default='developers-simple-key',
                        help='value for payload.auth_token')
    parser.add_argument('--audio-dir', metavar='PATH',
                        help='path to folder with used audio files')
    parser.add_argument('--texts', metavar='FILENAME',
                        help='path to file with texts for Vins.VoiceInput test')
    parser.add_argument('--tests-limit', metavar='NUM', type=int, default=100,
                        help='run NUM tests')
    parser.add_argument('--simultaneous-sessions', metavar='NUM', type=int, default=3,
                        help='use pool with NUM processes for execute tests')
    parser.add_argument('--sess-result', metavar='FILENAME', default='stress_out.jsonl',
                        help='save session results to JSONL file (sess per line) default=stress.out')
    parser.add_argument('--debug', action='store_true',
                        help='set loglevel DEBUG')
    context = parser.parse_args()

    if context.debug:
        root_logger.setLevel(logging.DEBUG)

    global UNIPROXY_URL
    UNIPROXY_URL = context.uniproxy
    global AUTH_TOKEN
    AUTH_TOKEN = context.auth_token
    with open(context.test_session) as f:
        test_script = json.load(f)
    for request_ref in ('vins_request', 'asr_request', 'bio_request'):
        try:
            test_type = get_action_by_ref(test_script, request_ref)['message']['event']['header']['name']
            break
        except:
            pass

    if test_type == 'VoiceInput' or test_type == 'Recognize' or test_type == 'Identify':
        list_audio = sorted(os.path.join(context.audio_dir, fn) for fn in os.listdir(context.audio_dir))
        audio_files = []  # one filename == one test session
        while len(audio_files) < context.tests_limit:
            audio_files += list_audio
        audio_files = audio_files[:context.tests_limit]
        test_params = audio_files
    elif test_type == 'TextInput':
        with open(context.texts) as f:
            texts = [line.strip() for line in f]
        test_params = []
        while len(test_params) < context.tests_limit:
            test_params += texts
        test_params = test_params[:context.tests_limit]
        random.shuffle(test_params)
    else:
        raise Exception('unsupported test type')

    ################################## TESTS ##################################
    pool = Pool(context.simultaneous_sessions)
    with open(context.sess_result, 'w') as f:
        for result in pool.imap_unordered(pool_func, ((num, param, test_script) for num, param in enumerate(test_params))):
            f.write(json.dumps(result, sort_keys=True, ensure_ascii=False))
            f.write('\n')
    return 0


if __name__ == '__main__':
    sys.exit(main())
