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
import re
import struct
import sys
import time
import uuid

from tornado.websocket import websocket_connect
from tornado.httpclient import HTTPRequest
from tornado.ioloop import IOLoop
from tornado import gen

from tests.checks import check_tree_contain_sample


logger = logging.getLogger(__name__)


def check_recognition_result(jmsg, recognition_sample):
    jrecognition = jmsg['directive']['payload']['recognition']
    recognition = []
    for r in jrecognition:
        recognition.append(' '.join(word['value'] for word in r['words']))
        break  # TODO: how handle multi-utterances?!
    logger.debug('recognition={}'.format(recognition))
    logger.debug('recognition_sample={}'.format(recognition_sample))
    if len(recognition_sample) < len(recognition): raise AssertionError(
        'recognized more utterances than expected')
    if len(recognition_sample) > len(recognition): raise AssertionError(
        'recognized less utterances than expected')
    for i in range(0, len(recognition_sample)):
        if isinstance(recognition_sample[i], list):
            assert recognition[i] in recognition_sample[i], \
                'bad recognition for utt[{}]: expected variants ({}) not contain "{}"'.format(
                    i, ', '.join('"{}"'.format(utt) for utt in recognition_sample[i]), recognition[i])
        else:
            assert recognition[i] == recognition_sample[i], \
                'bad recognition for utt[{}]: expected "{}" != "{}"'.format(
                    i, recognition_sample[i], recognition[i])


def check_normalized_result(jmsg, normalized_sample):
    jrecognition = jmsg['directive']['payload']['recognition']
    normalized_recognition = []
    for r in jrecognition:
        normalized_recognition.append(r['normalized'])
        break  # TODO: how handle multi-utterances?!
    logger.debug('recognition_normalized={}'.format(normalized_recognition))
    logger.debug('normalized_sample={}'.format(normalized_sample))
    if len(normalized_sample) < len(normalized_recognition): raise AssertionError(
        'recognized more utterances than expected')
    if len(normalized_sample) > len(normalized_recognition): raise AssertionError(
        'recognized less utterances than expected')
    for i in range(0, len(normalized_sample)):
        if isinstance(normalized_sample[i], list):
            assert normalized_recognition[i] in normalized_sample[i], \
                'bad norm. recognition for utt[{}]: expected variants ({}) not contain "{}"'.format(
                    i, ', '.join('"{}"'.format(utt) for utt in normalized_sample[i]), normalized_recognition[i])
        else:
            assert normalized_recognition[i] == normalized_sample[i], \
                'bad norm. recognition for utt[{}]: expected "{}" != "{}"'.format(
                    i, normalized_sample[i], normalized_recognition[i])


def check_sample_card_text(jmsg, text):
    jtext = jmsg['directive']['payload']['response']['card']['text']
    if isinstance(text, list):
        assert jtext in text, 'bad vind card text: expected variants ({}) not contain "{}"'.format(
            ', '.join('"{}"'.format(t) for t in text), jtext)
    else:
        assert jtext == text, 'bad vind card text: expected "{}" != "{}"'.format(text, jtext)


def check_sample_card_text_regexp(jmsg, pattern):
    jtext = jmsg['directive']['payload']['response']['card']['text']
    if isinstance(pattern, list):
        assert re.match(pt, jtext) is not None, 'bad vind card text: expected patterns variants ({}) not cover "{}"'.format(
            ', '.join('"{}"'.format(t) for pt in patttern), jtext)
    else:
        assert re.match(pattern, jtext) is not None, 'bad vind card text: expected "{}" != "{}"'.format(pattern, jtext)


def check_words(jmsg, words):
    words_info = jmsg['directive']['payload']['words']
    collected_words = []
    for w in words_info:
        collected_words.append(w['word'])
    assert collected_words == words, 'Bad tts chunking: expected "{}" != "{}"'.format(words, collected_words)


def check_response(jmsg, rec):
    """ check/validate received message if need (use samples from 'rec')
    """
    sample = rec.get('sample')
    if sample is not None:
        check_tree_contain_sample(jmsg, sample)
    recognition_sample = rec.get('recognition_sample')
    if recognition_sample is not None:
        check_recognition_result(jmsg, recognition_sample)
    sample_card_text = rec.get('sample_card_text')
    if sample_card_text is not None:
        check_sample_card_text(jmsg, sample_card_text)
    sample_card_text_regexp = rec.get('sample_card_text_regexp')
    if sample_card_text_regexp is not None:
        check_sample_card_text_regexp(jmsg, sample_card_text_regexp)
    normalized_sample = rec.get('normalized_sample')
    if normalized_sample:
        check_normalized_result(jmsg, normalized_sample)
    min_payload_size = rec.get('min_payload_size')
    if min_payload_size is not None:
        assert len(jmsg['directive']['payload']) >= min_payload_size, \
            'payload_size({}) < min_payload_size({})'.format(len(jmsg['directive']['payload']), min_payload_size)
    words_sample = rec.get('words_sample')
    if words_sample:
        check_words(jmsg, words_sample)


class DataStream:
    def __init__(self, sess, rec, stream_id):
        self.test_session = sess
        self.rec = rec
        self.stream_id = stream_id
        self.chunk_size = rec['chunk_size']
        self.chunk_duration = rec['chunk_duration']
        self.file = open(rec['filename'], 'rb')
        self.callback = IOLoop.current().call_later(self.chunk_duration, self.send_next_chunk)
        self.stopped = False

    def send_next_chunk(self):
        self.callback = None
        if self.stopped:
            return
        chunk = self.file.read(self.chunk_size)
        if chunk:
            if self.test_session.send_binary_message(chunk, stream_id=self.stream_id):
                self.callback = IOLoop.current().call_later(self.chunk_duration, self.send_next_chunk)
        else:
            finalize = self.rec.get('finalize')
            if finalize:
                actions = []
                if isinstance(finalize, dict):
                    actions = [finalize, ]
                elif isinstance(finalize, list):
                    actions = finalize
                for action in actions:
                    if action.get('action') == 'send_message':
                        self.test_session.send_message(action)
                    elif action.get('action') == 'async_send_data':
                        self.test_session.start_data_stream(action)

    def stop(self):
        self.stopped = True
        if self.callback:
            IOLoop.current().remove_timeout(self.callback)
            self.callback = None


class ReceivedData:
    def __init__(self, chunk):
        self.data = chunk
        self.first_chunk_size = len(chunk)
        self.start_stream_time = time.time()  # or timestamp
        self.last_chunk_time = self.start_stream_time

    def on_receive_chunk(self, chunk):
        self.data += chunk
        self.last_chunk_time = time.time()

    def bytes_per_second(self):
        dur = self.last_chunk_time - self.start_stream_time
        if dur < 0.0000001:
            return 0
        return (len(self.data) - self.first_chunk_size) / dur

    def size(self):
        return len(self.data)


class TestSession:
    def __init__(self, conn, script, auth_token, script_folder):
        self.conn = conn
        self.script = script
        self.auth_token = auth_token
        self.yamb_token = 'YambAuth fake-yamb-token'
        self.script_folder = script_folder

        self.stream = None
        self.stream_id = -1
        self.test_timeout = None
        self.error = None
        self.received_data = {}
        self.ignore_samples = {}
        self.require_only_one_samples = {}

        self.actions_lib = {}

    @gen.coroutine
    def process(self):
        logger.debug('begin process test script len={}'.format(len(self.script)))
        try:
            yield self.process_loop()
        finally:
            self.stop_data_stream()

    @gen.coroutine
    def process_loop(self):
        action_idx = 0
        for rec in self.script:
            action = rec['action']
            logger.debug('process test script action[{}]={}'.format(action_idx, action))
            while True:
                self.on_start_action(rec)
                if action == 'send_message':
                    msg = rec['message']
                    self.message_patcher(msg, rec.get('apply_to_message', []))
                    msg = json.dumps(msg)
                    logger.debug('write_message:\n{}'.format(msg))
                    yield self.conn.write_message(msg)
                elif action == 'recv_message':
                    msg = yield self.conn.read_message()
                    if msg is None:
                        logger.debug('got EOF')
                        if self.error:
                            raise Exception(self.error)
                        raise Exception('unexpected EOF (on recv_message)')
                    elif isinstance(msg, bytes):
                        self.on_recv_binary_data(msg)
                        continue
                    else:
                        if not self.process_message(rec, msg):
                            continue
                    self.on_recv_message(rec, msg)
                elif action == 'try_recv_message':
                    try:
                        msg = yield gen.with_timeout(datetime.timedelta(seconds=rec['seconds']), self.conn.read_message())
                        if isinstance(msg, bytes):
                            self.on_recv_binary_data(msg)
                        else:
                            self.process_message(rec, msg)
                    except Exception as exc:
                        pass
                elif action == 'async_send_data':
                    self.start_data_stream(rec)
                elif action == 'async_recv_message':
                    if rec.get('only_one_is_required', False):
                        self.require_only_one_samples[rec['id']] = rec['sample']
                    else:
                        self.ignore_samples[rec['id']] = rec['sample']
                elif action == 'set_timeout':
                    if self.test_timeout:
                        IOLoop.current().remove_timeout(self.test_timeout)
                        self.test_timeout = None
                    self.test_timeout = IOLoop.current().call_later(rec['timelimit'], self.on_test_timeout)
                elif action == 'load_library':
                    with open(os.path.join(self.script_folder, rec['filename'])) as f:
                        jdata = json.load(f)
                        for action_ in jdata:
                            action_['name']
                            self.actions_lib[action_['name']] = action_
                elif action == 'from_library':
                    logger.debug('use library action={}'.format(rec['use']))
                    rec = self.actions_lib[rec['use']]
                    action = rec['action']
                    continue
                elif action == 'check_received_data':
                    self.check_received_data(rec)
                elif action == 'sleep':
                    yield gen.sleep(rec['seconds'])
                else:
                    raise Exception('unknown test script action={}'.format(action))
                # stop async operations
                break
            action_idx += 1
        assert len(self.require_only_one_samples) == 0, \
            'not all async required data received, - left: {}'.format(', '.join('id:{}'.format(aid) for aid in self.require_only_one_samples))

    def process_message(self, rec, msg, check=True):
        logger.debug('read_message:\n{}'.format(msg))
        jmsg = json.loads(msg)
        recv_sync_message = False
        for aid, sample in self.ignore_samples.items():
            try:
                check_tree_contain_sample(jmsg, sample)
                recv_sync_message = True
                break
            except:
                pass
        if recv_sync_message:
            return False
        for aid, sample in self.require_only_one_samples.items():
            try:
                check_tree_contain_sample(jmsg, sample)
                recv_sync_message = True
                del self.require_only_one_samples[aid]
                break
            except:
                pass
        if recv_sync_message:
            return False
        if not check:
            return True
        try:
            check_response(jmsg, rec)
        except Exception as err:
            logger.error('received message not like sample: {}\n  msg={}\n  sample={}\n'.format(
                err,
                json.dumps(jmsg, ensure_ascii=False, sort_keys=True, indent=4),
                json.dumps(rec['sample'], ensure_ascii=False, sort_keys=True, indent=4),
            ))
            # stop async operations
            raise
        return True

    def on_start_action(self, rec):
        pass

    def on_recv_message(self, rec, message):
        pass

    def on_recv_binary_data(self, msg):
        logger.debug('got_binary_data len={}'.format(len(msg)))
        stream_id = struct.unpack(">I", msg[:4])[0]
        if stream_id not in self.received_data:
            self.received_data[stream_id] = ReceivedData(msg[4:])
        else:
            self.received_data[stream_id].on_receive_chunk(msg[4:])

    def check_received_data(self, rec):
        recv_data_size = self.received_data[rec['stream_id']].size()
        bytes_per_second = self.received_data[rec['stream_id']].bytes_per_second()
        has_check = True
        min_binary_data_size = rec.get('min_binary_data_size')
        if min_binary_data_size is not None:
            logger.debug('check recv_data_size={} >= min_binary_data_size={}'.format(
                recv_data_size, min_binary_data_size))
            assert min_binary_data_size <= recv_data_size, \
                'input stream data size to small'
            has_check = True
        max_binary_data_size = rec.get('max_binary_data_size')
        if max_binary_data_size is not None:
            logger.debug('check recv_data_size={} <= max_binary_data_size={}'.format(
                recv_data_size, max_binary_data_size))
            assert max_binary_data_size >= recv_data_size, \
                'input stream data size to large'
            has_check = True
        if not has_check:
            raise Exception('invalid check_received_data action (no checks)')
        min_bytes_per_second = rec.get('min_bytes_per_second')
        if min_bytes_per_second is not None:
            logger.debug('check bytes_per_second={} > min_bytes_per_second={}'.format(
                bytes_per_second, min_bytes_per_second))
            assert min_bytes_per_second < bytes_per_second, \
                'input stream data rate to small'
            has_check = True
        max_bytes_per_second = rec.get('max_bytes_per_second')
        if max_bytes_per_second is not None:
            logger.debug('check bytes_per_second={} < max_bytes_per_second={}'.format(
                bytes_per_second, max_bytes_per_second))
            assert max_bytes_per_second > bytes_per_second, \
                'input stream data rate to large'
            has_check = True

    def start_data_stream(self, rec):
        self.stream = DataStream(self, rec, self.stream_id)

    def stop_data_stream(self):
        if self.stream:
            self.stream.stop()
            self.stream = None

    def message_patcher(self, msg, patchs):
        for patch in patchs:
            if patch == 'new_stream_id':
                self.stream_id += 2
                msg['event']['header']['streamId'] = self.stream_id
            elif patch == 'new_message_id':
                self.message_id = str(uuid.uuid4())
                if 'streamcontrol' in msg:
                    msg['streamcontrol']['messageId'] = self.message_id
                else:
                    msg['event']['header']['messageId'] = self.message_id
            elif patch == 'new_uuid':
                self.uuid = "f" * 16 + str(uuid.uuid4()).replace("-", "")[16:32]
                msg['event']['payload']['uuid'] = self.uuid
            elif patch == 'old_stream_id':
                if 'streamcontrol' in msg:
                    msg['streamcontrol']['streamId'] = self.stream_id
                else:
                    assert False, 'can apply old stream_id only to streamcontrol'
            elif patch == 'auth_token':
                msg['event']['payload']['auth_token'] = self.auth_token
            elif patch == 'yamb_token':
                msg['event']['payload']['oauth_token'] = self.yamb_token
            elif patch == 'old_message_id':
                if 'streamcontrol' in msg:
                    msg['streamcontrol']['messageId'] = self.message_id
                else:
                    msg['event']['header']['messageId'] = self.message_id
            elif patch == 'old_uuid':
                msg['event']['payload']['uuid'] = self.uuid
            elif patch == 'application_client_time':
                now = datetime.datetime.now()
                msg['event']['payload']['application']['client_time'] = now.strftime("%Y%m%dT%H%M%S")
            elif patch == 'application_timestamp':
                now = datetime.datetime.now()
                msg['event']['payload']['application']['timestamp'] = str(int(now.timestamp()))
            elif patch == 'new_request_id':
                msg['event']['payload']['header']['request_id'] = str(uuid.uuid4())
            elif patch == 'new_vins_application_uuid':
                msg['event']['payload']['vins']['application']['uuid'] = "f" * 16 + str(uuid.uuid4()).replace("-", "")[16:32]

            else:
                raise Exception('unsupported message patch: {}'.format(patch))

    def send_message(self, rec):
        if not self.conn:
            logger.debug('skip send_message (to closed WS)')
            return
        msg = rec['message']
        self.message_patcher(msg, rec.get('apply_to_message', []))
        msg = json.dumps(msg)
        logger.debug('write_message:\n{}'.format(msg))
        self.conn.write_message(msg)

    def send_binary_message(self, data, stream_id=None):
        if not self.conn:
            logger.debug('skip send_binary_message (to closed WS)')
            return False
        logger.debug('send binary message stream={} size={}'.format(stream_id, len(data)))
        prepend = struct.pack(">I", stream_id)
        try:
            self.conn.write_message(prepend + data, binary=True)
        except Exception as err:
            logger.warn('fail send audiodata: {}'.format(err))
            return False
        return True

    def on_test_timeout(self):
        if not self.error:
            self.error = 'test reach timeout limit'
        self.finalize()

    def finalize(self):
        logger.debug('finalize test session')
        if self.test_timeout:
            IOLoop.current().remove_timeout(self.test_timeout)
            self.test_timeout = None
        if self.conn:
            self.conn.close()
            self.conn = None


def process_test_session(uniproxy_ws_url, filename, auth_token, test_script=None, send_headers=False):
    if test_script is None:
        with open(filename) as f:
            try:
                test_script = json.load(f)
            except Exception as exc:
                logger.exception('Bad test-script json format: {}'.format(exc))
                raise

    errors = []
    @gen.coroutine
    def ioloop_main():
        conn = None
        try:
            logger.debug('connect to uniproxy={}'.format(uniproxy_ws_url))
            if send_headers:
                conn = yield websocket_connect(HTTPRequest(
                    uniproxy_ws_url,
                    headers={
                        "X-UPRX-AUTH-TOKEN": auth_token,
                        "X-UPRX-UUID": "f" * 16 + str(uuid.uuid4()).replace("-", "")[16:32],
                    }
                ))
                session = TestSession(conn, test_script, None, os.path.dirname(filename))
            else:
                conn = yield websocket_connect(uniproxy_ws_url)
                session = TestSession(conn, test_script, auth_token, os.path.dirname(filename))
            yield session.process()
            conn.close()
            yield gen.sleep(1)
        except Exception as e:
            logger.exception('ioloop_main failed')
            errors.append(str(e))
        if conn:
            conn.close()
        IOLoop.current().stop()

    IOLoop.current().call_later(0, ioloop_main)
    IOLoop.current().start()
    IOLoop.clear_current()
    if not errors:
        logger.info('SUCCESS: session processed without errors')
    else:
        logger.error('FAIL:\n{}'.format('\n'.join(errors)))
    return errors


def main():
    root_logger = logging.getLogger('')
    file_logger = logging.StreamHandler()
    file_logger.setFormatter(logging.Formatter('%(asctime)s %(levelname)-8s %(filename)s:%(lineno)d %(message)s'))
    root_logger.addHandler(file_logger)
    root_logger.setLevel(logging.DEBUG)

    parser = argparse.ArgumentParser()

    parser.add_argument('--uniproxy', metavar='URL', default='ws://localhost:8887/uni.ws',
            help='url for access to tested uniproxy, default ws://localhost:8887/uni.ws')
    parser.add_argument('--test-session', metavar='FILENAME',
                        help='read test session descirption from given filename')
    parser.add_argument('--auth-token', metavar='URL', default='developers-simple-key',
                        help='value for payload.auth_token')

    context = parser.parse_args()
    if context.test_session:
        return len(process_test_session(context.uniproxy, context.test_session, context.auth_token))

    return 0


if __name__ == '__main__':
    sys.exit(main())
