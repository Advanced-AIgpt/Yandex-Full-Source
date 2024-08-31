#!/usr/bin/env python
# -*- coding: utf-8 -*-

import rtlog
import time
from alice.rtlog.ut.python.eventlog_wrap import load_frames, reset_eventlog
import alice.rtlog.protos.rtlog_ev_pb2 as rtlog_ev
import library.cpp.eventlog.proto.events_extension_pb2 as eventlog
from google.protobuf.json_format import MessageToJson
from google.protobuf.internal.containers import MutableMapping
from google.protobuf.message import Message
import socket
import datetime
import os
import json
import collections
import sys
from os import path


class SimpleCheck:
    def check(self, val):
        assert False


class Between(SimpleCheck):
    def __init__(self, lower_bound, upper_bound):
        self.lower_bound = lower_bound * 1000000
        self.upper_bound = upper_bound * 1000000

    def check(self, val):
        assert int(val) >= self.lower_bound
        assert int(val) <= self.upper_bound


class NotEmpty(SimpleCheck):
    def check(self, val):
        assert val


class Contains(SimpleCheck):
    def __init__(self, *strings):
        self._strings = strings

    def check(self, val):
        for s in self._strings:
            assert s in val


class Utf8Of(SimpleCheck):
    def __init__(self, s):
        self._s = s

    def check(self, val):
        assert isinstance(val, str)
        assert unicode(val, 'utf-8') == self._s


class CallbackCheck(SimpleCheck):
    def __init__(self, callback):
        self._callback = callback

    def check(self, val):
        self._callback(val)


class Missing:
    pass


def get_message_id(ev_type):
    return ev_type.DESCRIPTOR.GetOptions().Extensions[eventlog.message_id]


def check_event(actual, expected):
    if isinstance(expected, dict):
        is_mapping = isinstance(actual, MutableMapping)
        for k in expected:
            e = expected[k]
            if isinstance(e, Missing):
                if is_mapping:
                    assert k not in actual
                else:
                    assert not actual.HasField(k)
            else:
                if is_mapping:
                    a = actual.get(k, None)
                else:
                    a = getattr(actual, k, None)
                    assert isinstance(actual, Message)
                    enum_type = actual.DESCRIPTOR.fields_by_name[k].enum_type
                    if enum_type:
                        a = enum_type.values_by_number[a].name
                check_event(a, e)
    elif isinstance(expected, (basestring, int)):
        assert actual == expected
    elif isinstance(expected, SimpleCheck):
        expected.check(actual)
    elif expected is None:
        assert actual is None
    else:
        raise ValueError('unexpected type [{0}]'.format(type(expected)))


def check_events(actual, expected, timestamps=None):
    assert len(actual) == len(expected)
    for i in xrange(len(actual)):
        actual_frame = actual[i]
        expected_frame = expected[i]
        assert actual_frame.id == expected_frame['id']
        expected_events = expected_frame['events']
        assert len(actual_frame.events) == len(expected_events)
        for j in xrange(len(actual_frame.events)):
            actual_event = actual_frame.events[j]
            expected_event = expected_events[j]
            ev_type = expected_event.get('type', rtlog_ev.LogEvent)
            assert actual_event.event_class == get_message_id(ev_type)
            if 'timestamp' in expected_event:
                expected_timestamp = expected_event.pop('timestamp')
                if isinstance(expected_timestamp, Between):
                    expected_timestamp.check(actual_event.timestamp)
                else:
                    assert actual_event.timestamp == expected_timestamp
            elif timestamps:
                timestamps.check(actual_event.timestamp)
            actual_ev = ev_type()
            actual_ev.ParseFromString(actual_event.data)
            try:
                check_event(actual_ev, expected_event.get('content', expected_event))
            except:
                print('FAIL actual_ev\n{0}'.format(MessageToJson(actual_ev)))
                raise


Token = collections.namedtuple("Token", ["req_ts", "reqid", "activation_id"])


def _parse_token(s):
    reqts, req_id, activation_id = s.split('$')
    return Token(int(reqts), req_id, activation_id)


class RTLogContext:
    def __init__(self, events,
                 file_name='rtlog-test-file',
                 service_name='rtlog-test-service',
                 async=True,
                 max_string_size=1000000,
                 flush_period=datetime.timedelta(seconds=1),
                 flush_size=1025 * 1024):
        self.rt_log = None
        self._events = events
        self._file_name = file_name
        self._service_name = service_name
        self._async = async
        self._max_string_size = max_string_size
        self._flush_period = flush_period
        self._flush_size = flush_size

    def __enter__(self):
        reset_eventlog()
        if path.exists(self._file_name):
            os.remove(self._file_name)
        rtlog.activate(self._file_name, self._service_name,
                       async=self._async,
                       flush_period=self._flush_period,
                       max_string_size=self._max_string_size,
                       flush_size=self._flush_size)
        assert rtlog.is_active()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.rt_log = None
        rtlog.deactivate()
        if path.exists(self._file_name):
            self._events[:] = load_frames(self._file_name)
            os.remove(self._file_name)


def test_simple():
    events = []
    start_time = time.time()
    with RTLogContext(events) as ctx:
        ctx.rt_log = rtlog.begin_request()
        ctx.rt_log('ev1', f1='str_val', f2=42)
        ctx.rt_log('ev2', f3='val3')
        ctx.rt_log.end_request()

        ctx.rt_log = rtlog.begin_request()
        ctx.rt_log('ev3', f=43)
        ctx.rt_log.end_request()
    finish_time = time.time()

    timestamps_check = Between(start_time, finish_time)
    activation_started_check = {
        'type': rtlog_ev.ActivationStarted,
        'content': {
            'InstanceDescriptor': {
                'ServiceName': 'rtlog-test-service',
                'HostName': socket.gethostname()
            },
            'ReqTimestamp': timestamps_check,
            'ReqId': NotEmpty(),
            'ActivationId': NotEmpty(),
            'Pid': os.getpid(),
            'Session': False,
            'Continue': False
        }
    }
    activation_finished_check = {
        'type': rtlog_ev.ActivationFinished,
        'content': {}
    }
    check_events(events,
                 [
                     {
                         'id': 1,
                         'events': [
                             activation_started_check,
                             {
                                 'Message': 'ev1',
                                 'Fields': {
                                     'f1': 'str_val',
                                     'f2': '42'
                                 }
                             },
                             {
                                 'Message': 'ev2',
                                 'Fields': {
                                     'f3': 'val3'
                                 }
                             },
                             activation_finished_check
                         ]
                     },
                     {
                         'id': 2,
                         'events': [
                             activation_started_check,
                             {
                                 'Message': 'ev3',
                                 'Fields': {
                                     'f': '43'
                                 }
                             },
                             activation_finished_check
                         ]
                     }],
                 timestamps_check)


def test_as_json():
    events = []
    with RTLogContext(events) as ctx:
        ctx.rt_log = rtlog.begin_request()
        ctx.rt_log('test-ev1', request=rtlog.AsJson(json.dumps({'f': 'v1'})))
        ctx.rt_log('test-ev2', request=rtlog.AsJson({'f': 'v2'}))
        ctx.rt_log('test-ev3', request=rtlog.AsJson([{'f': 'v3'}, {'f': 'v4'}]))
        ctx.rt_log.end_request()

    activation_started_check = {
        'type': rtlog_ev.ActivationStarted,
        'content': {}
    }
    activation_finished_check = {
        'type': rtlog_ev.ActivationFinished,
        'content': {}
    }
    check_events(events,
                 [
                     {
                         'id': 1,
                         'events': [
                             activation_started_check,
                             {
                                 'Message': 'test-ev1',
                                 'Fields': {
                                     'request': '{"f": "v1"}',
                                     'request__format': 'json'
                                 }
                             },
                             {
                                 'Message': 'test-ev2',
                                 'Fields': {
                                     'request': '{"f": "v2"}',
                                     'request__format': 'json'
                                 }
                             },
                             {
                                 'Message': 'test-ev3',
                                 'Fields': {
                                     'request': '[{"f": "v3"}, {"f": "v4"}]',
                                     'request__format': 'json'
                                 }
                             },
                             activation_finished_check
                         ]
                     }])


def test_ignore_not_activated():
    assert rtlog.is_active() is False

    rt_log = rtlog.begin_request()
    rt_log('test-ev1')
    rt_log('test-ev2')
    rt_log('test-ev3')
    rt_log.end_request()
    rt_log = None

    stats = rtlog.get_stats()
    assert stats.active_loggers_count == 0
    assert stats.events_count == 0
    assert stats.pending_bytes_count == 0
    assert stats.written_frames_count == 0
    assert stats.written_bytes_count == 0
    assert stats.errors_count == 0
    assert stats.shrinked_bytes_count == 0


def test_counters():
    events = []
    with RTLogContext(events, async=False) as ctx:
        assert rtlog.get_stats().active_loggers_count == 0

        ctx.rt_log = rtlog.begin_request()
        assert rtlog.get_stats().active_loggers_count == 1
        assert rtlog.get_stats().events_count == 1
        ctx.rt_log('test-ev1')
        assert rtlog.get_stats().events_count == 2
        ctx.rt_log('test-ev2')
        assert rtlog.get_stats().events_count == 3
        assert rtlog.get_stats().pending_bytes_count > len('test-ev1') + len('test-ev2')

        rt_log2 = rtlog.begin_request()
        assert rtlog.get_stats().active_loggers_count == 2
        rt_log2('test-ev3')
        rt_log2('test-ev4')
        rt_log2('test-ev5')
        assert rtlog.get_stats().events_count == 7
        assert rtlog.get_stats().pending_bytes_count > len('test-ev1') + len('test-ev2') + \
            len('test-ev3') + len('test-ev4') + len('test-ev5')

        ctx.rt_log.end_request()
        assert rtlog.get_stats().events_count == 8
        assert rtlog.get_stats().active_loggers_count == 2
        ctx.rt_log = None
        assert rtlog.get_stats().active_loggers_count == 1
        rt_log2.end_request()
        rt_log2 = None

        stats = rtlog.get_stats()
        assert isinstance(stats, rtlog.ClientStats)
        assert stats.active_loggers_count == 0
        assert stats.events_count == 9
        assert stats.pending_bytes_count == 0
        assert stats.written_frames_count == 2
        assert stats.written_bytes_count > len('test-ev1') * 5
        assert stats.errors_count == 0


def test_invalid_file_name(capfd):
    events = []
    with RTLogContext(events, file_name='nonexistent_dir/rtlog-test-file', async=False) as ctx:
        ctx.rt_log = rtlog.begin_request()
        ctx.rt_log('test-ev1')
        ctx.rt_log('test-ev2')
        ctx.rt_log.end_request()
        ctx.rt_log = None
        assert rtlog.get_stats().errors_count == 2

        ctx.rt_log = rtlog.begin_request()
        ctx.rt_log('test-ev3')
        ctx.rt_log.end_request()
        ctx.rt_log = None
        assert rtlog.get_stats().errors_count == 3
        assert rtlog.get_stats().written_frames_count == 0
        assert rtlog.get_stats().written_bytes_count == 0

        assert 'can\'t open "nonexistent_dir/rtlog-test-file"' in capfd.readouterr().err


def test_max_string_size():
    events = []
    with RTLogContext(events, max_string_size=29) as ctx:
        ctx.rt_log = rtlog.begin_request()
        ctx.rt_log('test-ev1', k=u'тестоваястрокавкодировкеутфвосемь')
        assert rtlog.get_stats().shrinked_bytes_count == 46
        ctx.rt_log('test-ev2', k='a' * 30)
        assert rtlog.get_stats().shrinked_bytes_count == 46 + 9
        ctx.rt_log('test-ev3', k=u'микро тест')
        assert rtlog.get_stats().shrinked_bytes_count == 46 + 9
        ctx.rt_log.end_request()

    activation_started_check = {
        'type': rtlog_ev.ActivationStarted,
        'content': {}
    }
    activation_finished_check = {
        'type': rtlog_ev.ActivationFinished,
        'content': {}
    }
    check_events(events,
                 [
                     {
                         'id': 1,
                         'events': [
                             activation_started_check,
                             {
                                 'Message': 'test-ev1',
                                 'Fields': {
                                     'k': Utf8Of(u'тестоваяст... (66)')
                                 }
                             },
                             {
                                 'Message': 'test-ev2',
                                 'Fields': {
                                     'k': 'a' * 21 + '... (30)'
                                 }
                             },
                             {
                                 'Message': 'test-ev3',
                                 'Fields': {
                                     'k': Utf8Of(u'микро тест')
                                 }
                             },
                             activation_finished_check
                         ]
                     }
                 ])

    with RTLogContext(events, max_string_size=3) as ctx:
        ctx.rt_log = rtlog.begin_request()
        ctx.rt_log('ev', k=u'тестоваястрокавкодировкеутфвосемь')
        ctx.rt_log.end_request()

    check_events(events,
                 [
                     {
                         'id': 1,
                         'events': [
                             activation_started_check,
                             {
                                 'Message': 'ev',
                                 'Fields': {
                                     'k': '... (66)'
                                 }
                             },
                             activation_finished_check
                         ]
                     }
                 ])


def test_child_activations():
    events = []
    with RTLogContext(events) as ctx:
        ctx.rt_log = rtlog.begin_request()
        token1 = ctx.rt_log.get_token()
        token2 = ctx.rt_log.log_child_activation_started(u'тестовый сервис 1')
        ctx.rt_log.log_child_activation_finished(token2, True)
        ctx.rt_log.end_request()

        ctx.rt_log = rtlog.begin_request(session=True)
        token3 = ctx.rt_log.get_token()
        child_logger = ctx.rt_log.create_request_logger(u'тестовый запрос 1')
        token4 = child_logger.get_token()
        token5 = child_logger.log_child_activation_started(u'тестовый сервис 2')
        child_logger.log_child_activation_finished(token5, False)
        child_logger.end_request()
        child_logger = None
        ctx.rt_log.end_request()

    check_events(events,
                 [
                     {
                         'id': 1,
                         'events': [
                             {
                                 'type': rtlog_ev.ActivationStarted,
                                 'content': {
                                     'ReqTimestamp': _parse_token(token1).req_ts,
                                     'ReqId': _parse_token(token1).reqid,
                                     'ActivationId': _parse_token(token1).activation_id,
                                     'Session': False,
                                     'Continue': False
                                 }
                             },
                             {
                                 'type': rtlog_ev.ChildActivationStarted,
                                 'content': {
                                     'ReqTimestamp': Missing(),
                                     'ReqId': Missing(),
                                     'ActivationId': _parse_token(token2).activation_id,
                                     'Description': Utf8Of(u'тестовый сервис 1')
                                 }
                             },
                             {
                                 'type': rtlog_ev.ChildActivationFinished,
                                 'content': {
                                     'ActivationId': _parse_token(token2).activation_id,
                                     'Ok': True
                                 }
                             },
                             {
                                 'type': rtlog_ev.ActivationFinished,
                                 'content': {}
                             }
                         ]
                     },
                     {
                         'id': 2,
                         'events': [
                             {
                                 'type': rtlog_ev.ActivationStarted,
                                 'content': {
                                     'ReqTimestamp': _parse_token(token4).req_ts,
                                     'ReqId': _parse_token(token4).reqid,
                                     'ActivationId': _parse_token(token4).activation_id,
                                     'Session': False,
                                     'Continue': False
                                 }
                             },
                             {
                                 'type': rtlog_ev.ChildActivationStarted,
                                 'content': {
                                     'ReqTimestamp': Missing(),
                                     'ReqId': Missing(),
                                     'ActivationId': _parse_token(token5).activation_id,
                                     'Description': Utf8Of(u'тестовый сервис 2')
                                 }
                             },
                             {
                                 'type': rtlog_ev.ChildActivationFinished,
                                 'content': {
                                     'ActivationId': _parse_token(token5).activation_id,
                                     'Ok': False
                                 }
                             },
                             {
                                 'type': rtlog_ev.ActivationFinished,
                                 'content': {}
                             }
                         ]
                     },
                     {
                         'id': 3,
                         'events': [
                             {
                                 'type': rtlog_ev.ActivationStarted,
                                 'content': {
                                     'ReqTimestamp': _parse_token(token3).req_ts,
                                     'ReqId': _parse_token(token3).reqid,
                                     'ActivationId': _parse_token(token3).activation_id,
                                     'Session': True,
                                     'Continue': False
                                 }
                             },
                             {
                                 'type': rtlog_ev.ChildActivationStarted,
                                 'content': {
                                     'ReqTimestamp': _parse_token(token4).req_ts,
                                     'ReqId': _parse_token(token4).reqid,
                                     'ActivationId': _parse_token(token4).activation_id,
                                     'Description': Utf8Of(u'тестовый запрос 1')
                                 }
                             },
                             {
                                 'type': rtlog_ev.ChildActivationFinished,
                                 'content': {
                                     'ActivationId': _parse_token(token4).activation_id,
                                     'Ok': True
                                 }
                             },
                             {
                                 'type': rtlog_ev.ActivationFinished,
                                 'content': {}
                             }
                         ]
                     }
                 ])


def test_log_errors():
    def raise_error(message):
        raise ValueError(message)

    events = []
    with RTLogContext(events) as ctx:
        ctx.rt_log = rtlog.begin_request()
        try:
            raise_error('test-exception-message1')
        except:
            ctx.rt_log.exception('test-error-1', attr='test-val1')

        exc_info = None
        try:
            raise_error('test-exception-message2')
        except:
            exc_info = sys.exc_info()
        ctx.rt_log.exception_info(exc_info, 'test-error-2', attr='test-val2')

        try:
            raise_error('test-exception-message3')
        except:
            ctx.rt_log.error('test-error-3', exc_info=1, attr='test-val3')

        test_error_event_timestamp = time.time()
        ctx.rt_log.log_event(test_error_event_timestamp, 'error', 'test-error-4', a=1, b=2)
        test_info_event_timestamp = time.time()
        ctx.rt_log.log_event(test_info_event_timestamp, 'info', 'test-info-message1', x=22)

        try:
            raise_error('test-exception-message4')
        except:
            ctx.rt_log('test-info-message2', exc_info=1, test_key='test_val')

        ctx.rt_log.end_request()

    check_events(events,
                 [
                     {
                         'id': 1,
                         'events': [
                             {
                                 'type': rtlog_ev.ActivationStarted,
                                 'content': {}
                             },
                             {
                                 'Message': 'test-error-1',
                                 'Severity': 'RTLOG_SEVERITY_ERROR',
                                 'Backtrace': Contains('raise_error', 'test-exception-message1'),
                                 'Fields': {
                                     'exc_info': Missing(),
                                     'attr': 'test-val1'
                                 }
                             },
                             {
                                 'Message': 'test-error-2',
                                 'Severity': 'RTLOG_SEVERITY_ERROR',
                                 'Backtrace': Contains('raise_error', 'test-exception-message2'),
                                 'Fields': {
                                     'exc_info': Missing(),
                                     'attr': 'test-val2'
                                 }
                             },
                             {
                                 'Message': 'test-error-3',
                                 'Severity': 'RTLOG_SEVERITY_ERROR',
                                 'Backtrace': Contains('raise_error', 'test-exception-message3'),
                                 'Fields': {
                                     'exc_info': Missing(),
                                     'attr': 'test-val3'
                                 }
                             },
                             {
                                 'timestamp': int(test_error_event_timestamp * 1000000),
                                 'Message': 'test-error-4',
                                 'Severity': 'RTLOG_SEVERITY_ERROR',
                                 'Backtrace': Missing(),
                                 'Fields': {
                                     'a': '1',
                                     'b': '2'
                                 }
                             },
                             {
                                 'timestamp': int(test_info_event_timestamp * 1000000),
                                 'Message': 'test-info-message1',
                                 'Severity': 'RTLOG_SEVERITY_INFO',
                                 'Backtrace': Missing(),
                                 'Fields': {
                                     'x': '22'
                                 }
                             },
                             {
                                 'Message': 'test-info-message2',
                                 'Severity': 'RTLOG_SEVERITY_INFO',
                                 'Backtrace': Contains('raise_error', 'test-exception-message4'),
                                 'Fields': {
                                     'test_key': 'test_val'
                                 }
                             },
                             {
                                 'type': rtlog_ev.ActivationFinished,
                                 'content': {}
                             }
                         ]
                     }])


def test_log_request_context():
    events = []
    with RTLogContext(events, max_string_size=10) as ctx:
        ctx.rt_log = rtlog.begin_request()
        ctx.rt_log.log_request_context(uuid='test-uuid', device_id='test-deviceid', session_id=None)
        ctx.rt_log.end_request()

    check_events(events,
                 [
                     {
                         'id': 1,
                         'events': [
                             {
                                 'type': rtlog_ev.ActivationStarted,
                                 'content': {}
                             },
                             {
                                 'type': rtlog_ev.CreateRequestContext,
                                 'content': {
                                     'Fields': {
                                         'uuid': 'test-uuid',
                                         'device_id': 'te... (13)',
                                         'session_id': Missing()
                                     }
                                 }
                             },
                             {
                                 'type': rtlog_ev.ActivationFinished,
                                 'content': {}
                             }
                         ]
                     }])


def test_backtrace_limit():
    limit = 20

    def raise_error(message, depth):
        if depth == 1:
            raise ValueError(message)
        raise_error(message, depth - 1)

    def check_backtrace(backtrace):
        expected_count = (limit - 1) * 2 + 1
        assert backtrace.count('raise_error') == expected_count

    events = []
    with RTLogContext(events) as ctx:
        ctx.rt_log = rtlog.begin_request()
        try:
            raise_error('test-exception-message', limit * 2)
        except:
            ctx.rt_log.exception('test-error', attr='test-val1')
        ctx.rt_log.end_request()

    check_events(events,
                 [
                     {
                         'id': 1,
                         'events': [
                             {
                                 'type': rtlog_ev.ActivationStarted,
                                 'content': {}
                             },
                             {
                                 'Message': 'test-error',
                                 'Backtrace': CallbackCheck(check_backtrace)
                             },
                             {
                                 'type': rtlog_ev.ActivationFinished,
                                 'content': {}
                             }
                         ]
                     }])


def test_log_proto_event():
    events = []
    with RTLogContext(events, max_string_size=8) as ctx:
        ctx.rt_log = rtlog.begin_request()
        ctx.rt_log.log_proto_event(rtlog_ev.LogEvent, Message='123456789')
        ev = rtlog_ev.LogEvent()
        ev.Message = 'm2'
        ev.Fields['f1'] = 'greater_than_limit__is_this_a_problem?'
        ctx.rt_log.log_proto_event(ev)
        ctx.rt_log.end_request()

    check_events(events,
                 [
                     {
                         'id': 1,
                         'events': [
                             {
                                 'type': rtlog_ev.ActivationStarted,
                                 'content': {}
                             },
                             {
                                 'Message': '1... (9)'
                             },
                             {
                                 'Message': 'm2',
                                 'Fields': {
                                     'f1': 'greater_than_limit__is_this_a_problem?'
                                 }
                             },
                             {
                                 'type': rtlog_ev.ActivationFinished,
                                 'content': {}
                             }
                         ]
                     }])


def test_flush_frames_by_size():
    events = []
    flush_size = 500

    def write_event(message):
        with RTLogContext(events, flush_period=datetime.timedelta(milliseconds=20), flush_size=flush_size) as ctx:
                ctx.rt_log = rtlog.begin_request()
                ctx.rt_log(message)
                time.sleep(0.015)
                ctx.rt_log.end_request()

    write_event(message='test')
    check_events(events,
                 [
                     {
                         'id': 1,
                         'events': [
                             {
                                 'type': rtlog_ev.ActivationStarted,
                                 'content': {
                                     'Continue': False
                                 }
                             },
                             {
                                 'Message': 'test'
                             },
                             {
                                 'type': rtlog_ev.ActivationFinished,
                                 'content': {}
                             }
                         ]
                     }])

    write_event(message='test_' + ('a' * flush_size))
    check_events(events,
                 [
                     {
                         'id': 1,
                         'events': [
                             {
                                 'type': rtlog_ev.ActivationStarted,
                                 'content': {
                                     'Continue': False
                                 }
                             },
                             {
                                 'Message': 'test_' + ('a' * flush_size)
                             }
                         ]
                     },
                     {
                         'id': 2,
                         'events': [
                             {
                                 'type': rtlog_ev.ActivationStarted,
                                 'content': {
                                     'Continue': True
                                 }
                             },
                             {
                                 'type': rtlog_ev.ActivationFinished,
                                 'content': {}
                             }
                         ]
                     }])


def test_can_fork(capfd):
    # Продолжать пользоваться одним и тем же экземпляром логгера после
    # форка в обоих процессах - странная затея и скорей всего баг в клиентском коде.
    # Этим тестом фиксируем least surprise в этом случае.

    events = []
    with RTLogContext(events, async=False) as ctx:
        ctx.rt_log = rtlog.begin_request()
        ctx.rt_log('ev1')
        child_pid = os.fork()
        if child_pid == 0:
            ctx.rt_log('ev2')
            ctx.rt_log.end_request()
            ctx.rt_log = None

            sys.stderr.write('child process done')
            sys.stderr.flush()

            os._exit(0)
        pid, status = os.waitpid(child_pid, 0)
        assert os.WIFEXITED(status) and os.WEXITSTATUS(status) == 0
        ctx.rt_log('ev3')
        ctx.rt_log.end_request()

    assert 'child process done' in capfd.readouterr().err
    check_events(events,
                 [
                     {
                         'id': 1,
                         'events': [
                             {
                                 'type': rtlog_ev.ActivationStarted,
                                 'content': {}
                             },
                             {
                                 'Message': 'ev1'
                             },
                             {
                                 'Message': 'ev2'
                             },
                             {
                                 'type': rtlog_ev.ActivationFinished,
                                 'content': {}
                             }
                         ]
                     },
                     {
                         'id': 1,
                         'events': [
                             {
                                 'type': rtlog_ev.ActivationStarted,
                                 'content': {}
                             },
                             {
                                 'Message': 'ev1'
                             },
                             {
                                 'Message': 'ev3'
                             },
                             {
                                 'type': rtlog_ev.ActivationFinished,
                                 'content': {}
                             }
                         ]
                     }
                 ])
