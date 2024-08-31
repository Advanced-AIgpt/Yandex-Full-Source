#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# VOICESERV-1450 monitoring script for regular checking uniproxy(asr,vins) functions
#
import argparse
import datetime
import json
import logging
import os
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


def get_action_by_ref(script, action_ref):
    for action in script:
        aref = action.get('action_ref')
        if aref is not None and aref == action_ref:
            return action
    return None


def ensure_positive(val):
    return val if val >= 0 else 0


class SessionContext:
    def __init__(self, uniproxy_ws_url, auth_token, voice_template, text_template, enrich_mode):
        self.uniproxy_ws_url = uniproxy_ws_url
        self.auth_token = auth_token
        self.voice_template = voice_template
        self.text_template = text_template
        self.enrich_mode = enrich_mode
        self.sessions_processed = 0
        self.failed_sessions = []


class VinsTestSession(TestSession):
    def __init__(self, conn, script, params, session_context, script_folder):
        TestSession.__init__(self, conn, script, session_context.auth_token, script_folder)
        self.start_session = time.time()
        self.session_context = session_context
        self.params = params

    def check_received_data(self, rec):
        super().check_received_data(rec)
        if not self.session_context.enrich_mode:
            return

        valid_diff = 0.15
        curr_tts_size = len(self.received_data[rec['stream_id']])

        curr_min_tts_size = int(curr_tts_size * (1 - valid_diff))
        curr_max_tts_size = int(curr_tts_size * (1 + valid_diff))
        min_tts_size = self.params.get('min_tts_size')
        if min_tts_size is None or curr_min_tts_size < min_tts_size:
            self.params['min_tts_size'] = curr_min_tts_size
        max_tts_size = self.params.get('max_tts_size')
        if max_tts_size is None or curr_max_tts_size > max_tts_size:
            self.params['max_tts_size'] = curr_max_tts_size


class VinsVoiceInputTestSession(VinsTestSession):
    def __init__(self, conn, script, params, session_context, script_folder):
        VinsTestSession.__init__(self, conn, script, params, session_context, script_folder)
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
                if self.session_context.enrich_mode:
                    jmsg = json.loads(message)
                    jrecognition = jmsg['directive']['payload']['recognition']
                    for r in jrecognition:
                        normalized = r['normalized']
                        nr = self.params.get('normalized_recognition')
                        if nr is None:
                            self.params['normalized_recognition'] = normalized
                        elif isinstance(nr, str) and nr != normalized:
                            self.params['normalized_recognition'] = [nr, normalized]
                        else:
                            if normalized not in nr:
                                nr.append(normalized)
                        break  # TODO: how handle multi-utterances?!
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
    def __init__(self, conn, script, params, session_context, script_folder):
        VinsTestSession.__init__(self, conn, script, params, session_context, script_folder)
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


def process_session(context, params, sess_num):
    sess_start = time.time()
    voice = params.get('voice')
    if not voice:
        text = params.get('text')
        if text is None:
            raise Exception('invalid params={} (no contaion text or voice)'.format(params))

    if voice:
        session_creator = VinsVoiceInputTestSession
        test_script = deepcopy(context.voice_template)
        audio_action = get_action_by_ref(test_script, 'send_audio_file')
        audio_action['filename'] = voice
        if not context.enrich_mode:
            nr = params.get('normalized_recognition')
            if nr:
                asr_result = get_action_by_ref(test_script, 'asr_result')
                asr_result['normalized_sample'] = [ nr ]
            check_received_data = get_action_by_ref(test_script, 'check_received_data')
            min_tts_size = params.get('min_tts_size')
            if min_tts_size is not None:
                check_received_data['min_binary_data_size'] = min_tts_size
                logger.info('set min TTS size')
            max_tts_size = params.get('max_tts_size')
            if max_tts_size is not None:
                check_received_data['max_binary_data_size'] = max_tts_size
    else:  # text input
        session_creator = VinsTextInputTestSession
        test_script = deepcopy(context.text_template)
        action = get_action_by_ref(test_script, 'vins_request')
        action['message']['event']['payload']['request']['event']['text'] = text

    timelimit = get_action_by_ref(test_script, 'timelimit')
    enrich_mode_timelimit = 42
    if context.enrich_mode and timelimit['timelimit'] < enrich_mode_timelimit:
        timelimit['timelimit'] = 42

    timelimit = get_action_by_ref(test_script, 'timelimit')
    errors = []
    timings = {
        'start_test': sess_start
    }
    @gen.coroutine
    def ioloop_main():
        logger.info("run job={} param={}".format(sess_num, params))
        conn = None
        session = None
        try:
            logger.debug('connect to uniproxy={}'.format(context.uniproxy_ws_url))
            conn = yield websocket_connect(context.uniproxy_ws_url)
            session = session_creator(conn, test_script, params, context, os.path.dirname(__file__))
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
    if errors:
        logger.error('job={} failed:\n{}'.format(sess_num, '\n'.join(errors)))
    timings['test_duration'] = ensure_positive(time.time() - sess_start)
    context.sessions_processed += 1
    if errors:
        context.failed_sessions.append({ 'sess_num': sess_num, 'name': params.get('name', ''), 'errors': errors })


def main():
    root_logger = logging.getLogger('')
    file_logger = logging.StreamHandler()
    file_logger.setFormatter(logging.Formatter('%(asctime)s %(levelname)-8s %(filename)s:%(lineno)d %(message)s'))
    root_logger.addHandler(file_logger)
    root_logger.setLevel(logging.INFO)

    parser = argparse.ArgumentParser()

    parser.add_argument('--uniproxy', metavar='URL', default='wss://uniproxy.tst.voicetech.yandex.net/uni.ws',
                        help='url for access to tested uniproxy (wss://uniproxy.tst.voicetech.yandex.net/uni.ws)')
    parser.add_argument('--test-ammo', metavar='FILENAME',
                        help='read test ammo (params for test-session template) (format=JSONL)')
    parser.add_argument('--test-voice-session', metavar='FILENAME', default='monitoring_vins_voice_session.json',
                        help='read test session VINS.VoiceInput template from given filename')
    parser.add_argument('--test-text-session', metavar='FILENAME', default='monitoring_vins_text_session.json',
                        help='read test session VINS.TextInput template from given filename')
    parser.add_argument('--auth-token', metavar='URL', default='developers-simple-key',
                        help='value for payload.auth_token')
    parser.add_argument('--report', metavar='FILENAME', default='monitoring_report.json',
                        help='save test sessions statistic to JSON file default=monitoring_report.json')
    parser.add_argument('--enriched-ammo', metavar='FILENAME',
                        help='append/update asr result & tts outpus size to input test session and store to given file')
    parser.add_argument('--debug', action='store_true',
                        help='set loglevel DEBUG')
    context = parser.parse_args()

    if context.debug:
        root_logger.setLevel(logging.DEBUG)

    with open(context.test_voice_session) as f:
        voice_session_template = json.load(f)

    with open(context.test_text_session) as f:
        text_session_template = json.load(f)

    fea = None
    session_context = None
    try:
        if context.enriched_ammo:
             fea = open(context.enriched_ammo, 'w')

        session_context = SessionContext(
            context.uniproxy,
            context.auth_token,
            voice_session_template,
            text_session_template,
            fea is not None,
        )
        sess_num = 0
        with open(context.test_ammo) as f:
            for line in f:
                line = line.strip()
                if not line:
                    continue
                session_params = json.loads(line)
                process_session(session_context, session_params, sess_num)  # <<< execute singe session
                sess_num += 1
                if fea:
                    rec = json.dumps(session_params, ensure_ascii=False, sort_keys=True)
                    logger.info('save enriched: {}'.format(rec))
                    fea.write(rec + '\n')  # JSONL
    finally:
        if fea:
            fea.close()
    if session_context:
        with open(context.report, 'w') as f:
            f.write(json.dumps(
                {
                    'sessions_processed': session_context.sessions_processed,
                    'failed_sessions': session_context.failed_sessions,
                },
                ensure_ascii=False,
                sort_keys=True,
                indent=4,
            ))

    return 0


if __name__ == '__main__':
    sys.exit(main())
