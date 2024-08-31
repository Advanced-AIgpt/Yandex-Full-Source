#!/usr/bin/env python
# encoding: utf-8

import copy
import functools
import random
import requests
import uuid
import json
import re
import datetime
import time
import pytest

from alice.uniproxy.library.backends_bio.yabio_storage import YabioStorage, ContextStorage, connect_ydb, get_table_name
from voicetech.library.proto_api.yabio_pb2 import YabioContext, YabioVoiceprint
from collections import namedtuple
from alice.uniproxy.library.events importStreamControl
from functools import partial as partial_tmpl  # simakazi@ already use partial for varname :)
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.settings import config
from tests.basic import connect as connect_and_run_ioloop, server, exit_ioloop, spawn_all
from tests.basic import write_json, write_data, read_message, write_stream, write_streamcontrol, read_stream
from tornado.ioloop import IOLoop
from tornado import gen

BassTestUser = namedtuple("TestUser", ["login", "uuid", "token", "client_ip"])

BASS_URL = "http://bass-testing.n.yandex-team.ru/"
POSTMAN_TOKEN = "e08a85c5-7be5-46f2-a528-666e99c17a5a"
TEST_USER_TIMEOUT = 300
BIOMETRY_GROUP = "my_fancy_biometry_test_do_not_reuse-{}".format(random.randint(1,100))
TEST_SESSION = 'test-session'
PHRASE_CNT = 5
DEV_MODEL = "Station"
DEV_MANUFACTURER = "Yandex"


SYNC_STATE = {
    "event": {
        "header": {
            "namespace": "System",
            "name": "SynchronizeState",
            "messageId": str(uuid.uuid4()),
        },
        "payload": {
            "auth_token": config["key"],
            "uuid": "f" * 16 + str(uuid.uuid4()).replace("-", "")[16:32],
            "vins": {
                "application": {
                    "app_id": "ru.yandex.quasar.app",
                    "device_manufacturer": DEV_MANUFACTURER,
                    "device_model": DEV_MODEL,
                    "app_version": "1.0",
                    "os_version": "5.0",
                    "platform": "android",
                    "uuid": "f" * 16 + str(uuid.uuid4()).replace("-", "")[16:32],
                    "lang": "ru-RU",
                },
            },
        }
    }
}


VOICE_INPUT = {
    "event": {
        "header": {
            "namespace": "VINS",
            "name": "VoiceInput",
            "messageId": str(uuid.uuid4()),
            "streamId": 1
        },
        "payload": {
            "header": {
                "request_id": str(uuid.uuid4()),
                "state_id": 0,
                "sequence_number": 42,
                "try_id": 0
            },
            "request": {
                "event": {
                    "type": "voice_input",
                    "voice_session": True,
                },
                "device_state": {
                    'device_id': 'TEST_DEVICE_ID',
                },
                "experiments": [
                    "enable_biometry_scoring",
                    "personalization",
                    "quasar_biometry",
                    "quasar_biometry_new_enroll",
                    "biometry_remove"
                ],
                "additional_options": {
                    "bass_url": BASS_URL,
                },
            },
            "application": {
                "client_time": datetime.datetime.now().strftime("%Y%m%dT%H%M%S"),
                "timezone": "Europe/Moscow",
                "timestamp": "%d" % (datetime.datetime.now().timestamp()),
            },
            "lang": "ru-RU",
            "topic": "desktopgeneral",
            "format": "audio/opus",
            "key": "developers-simple-key",
            "advancedASROptions": {
                "partial_results": True,
                "utterance_silence": 120
            },
            "biometry_group": BIOMETRY_GROUP,
            "vins_scoring": True,
            "biometry_score": True,
            "vins_partial": True
        }
    }
}


def new_voice_input():
    VOICE_INPUT["event"]["header"]["messageId"] = str(uuid.uuid4())
    VOICE_INPUT["event"]["payload"]["header"]["state_id"] += 1
    VOICE_INPUT["event"]["payload"]["header"]["sequence_number"] += 1
    VOICE_INPUT["event"]["payload"]["header"]["prev_req_id"] = VOICE_INPUT["event"]["payload"]["header"]["request_id"]
    VOICE_INPUT["event"]["payload"]["header"]["request_id"] = str(uuid.uuid4())
    return copy.deepcopy(VOICE_INPUT)


def _get_bass_test_user() -> BassTestUser:
    response = requests.post(
        BASS_URL + "test_user",
        json={
            "args": {
                "tags": ["oauth"],
            },
            "method": "GetUser",
            "timeout": TEST_USER_TIMEOUT,
        },
        headers={
            "Postman-Token": POSTMAN_TOKEN,
            "Cache-Control": "no-cache"
        }
    )
    assert(response.status_code == 200)
    response = response.json()

    test_user = BassTestUser(
        login=response["result"]["login"],
        client_ip=response["result"]["client_ip"] or "127.0.0.1",
        token=response["result"]["token"],
        uuid=response["result"]["uuid"],
    )

    return test_user


def _free_bass_test_user(login: str):
    response = requests.post(
        BASS_URL + "test_user",
        json={
            "method": "FreeUser",
            "args": {
                "login": login,
            },
        },
        headers={
            "Postman-Token": POSTMAN_TOKEN,
            "Cache-Control": "no-cache"
        }
    )
    assert(response.status_code == 200)


def _test_voice_input(server, file_name, asr_text, vins_should_answer, partial=False, spotter_file_name=None, spotter_text=None):
    def _run(socket):
        _stop_streaming = False
        _complete = 0

        def stop_streaming():
            nonlocal _stop_streaming
            _stop_streaming = True

        def should_stop_streaming():
            nonlocal _stop_streaming
            return _stop_streaming

        def complete():
            nonlocal _complete
            _complete += 1
            if _complete == 2:
                Logger.get().debug('complete _test_voice_input')
                exit_ioloop()

        @gen.coroutine
        def _write():
            stream_id = 1
            write_json(socket, SYNC_STATE)

            voice_input = new_voice_input()
            if spotter_file_name:
                payload = voice_input['event']['payload']
                payload['enable_spotter_validation'] = True
                payload['spotter_phrase'] = spotter_text
                payload['data_to_spotter_end'] = 0
            write_json(socket, voice_input)

            if spotter_file_name:
                message_id = str(uuid.uuid4())
                Logger.get().debug('begin sending spotter audio')
                yield write_stream(
                    socket,
                    spotter_file_name,
                    stream_id,
                    message_id,
                    should_stop_streaming,
                    send_streamcontrol=False,
                    chunk_size=6400,
                    delay=0.4,
                )
                Logger.get().debug('sent spotter audio')
                yield write_streamcontrol(
                    socket,
                    stream_id,
                    message_id,
                    action=StreamControl.ActionType.SPOTTER_END,
                )
                Logger.get().debug('sent spotter-end streamcontrol')

            yield write_stream(
                socket,
                file_name,
                stream_id,
                str(uuid.uuid4()),
                should_stop_streaming,
                send_streamcontrol=True,
                chunk_size=6400,
                delay=0.4,
            )
            Logger.get().debug('sent audio')
            stop_streaming()
            Logger.get().debug('complete write')
            complete()

        @gen.coroutine
        def read_with_filter(socket, ignore=None):
            if ignore is None:
                ignore = []

            while True:
                message = yield read_message(socket)
                Logger.get().debug('FROM UNIPROXY ' + ('<' * 90))
                Logger.get().debug(json.dumps(message, ensure_ascii=False))
                directive = "{}.{}".format(
                    message["directive"]["header"]["namespace"],
                    message["directive"]["header"]["name"]
                )
                directive = directive.lower()
                if directive in ignore:
                    Logger.get().debug('ignore message')
                    continue
                Logger.get().debug('READ_WITH_FILTER exit')
                return message

        @gen.coroutine
        def read_assert(expected, socket):
            message = yield read_with_filter(socket, ignore=['spotter.validation'])
            directive = "{}.{}".format(
                message["directive"]["header"]["namespace"],
                message["directive"]["header"]["name"]
            )
            directive = directive.lower()
            expected = expected.lower()
            assert directive == expected, 'got unexpected={} directive expect={}'.format(directive, expected)
            return message

        @gen.coroutine
        def _read():
            while True:
                # read partials
                message = yield read_assert('ASR.Result', socket)
                if message["directive"]["payload"]["endOfUtt"]:
                    asr_recognized_text = message["directive"]["payload"]["recognition"][0]["normalized"]
                    Logger.get().info('ASR text: "{}"'.format(asr_recognized_text))
                    assert asr_text in asr_recognized_text, 'not found sample={} in recognition result={}'.format(
                        asr_text,
                        asr_recognized_text,
                    )
                    break
            stop_streaming()
            message = yield read_message(socket)
            assert message["streamcontrol"]["streamId"] == 1
            vins = yield read_assert('Vins.VinsResponse', socket)
            if vins_should_answer:
                vins_text = vins["directive"]["payload"]["response"]["card"]["text"]
                Logger.get().info('VINS text: "{}"'.format(vins_text))
                assert re.match(".*(" + vins_should_answer + ").*", vins_text)
            if vins_should_answer:
                message = yield read_assert('Tts.Speak', socket)
                speech_len, stream_control = (yield read_stream(socket))
                assert stream_control["streamcontrol"]["streamId"] == 2
                assert speech_len > 2000
            Logger.get().debug('complete read')
            complete()

        @gen.coroutine
        def _read_noeou():
            while True:
                message = yield read_message(socket)
                Logger.get().info(message)
                if "streamcontrol" in message:
                    streamId = message["streamcontrol"]["streamId"]
                    assert streamId == 1
                elif "directive" in message:
                    directive = "{}.{}".format(
                        message["directive"]["header"]["namespace"],
                        message["directive"]["header"]["name"]
                    )
                    directive = directive.lower()
                    if directive == "asr.result":
                        time_asr = time.time()
                        try:
                            asr_recognized_text = message["directive"]["payload"]["recognition"][0]["normalized"]
                            Logger.get().info('ASR text: "{}"'.format(asr_recognized_text))
                        except Exception as exc:
                            pass
                    elif directive == "vins.vinsresponse":
                        if vins_should_answer:
                            vins_text = message["directive"]["payload"]["response"]["card"]["text"]
                            Logger.get().info('VINS text: "{}"'.format(vins_text))
                            assert re.match(".*(" + vins_should_answer + ").*", vins_text)
                        else:
                            Logger.get().debug('complete read_noeou (skip tts)')
                            complete()
                            break
                    elif directive == "tts.speak":
                        speech_len, stream_control = (yield read_stream(socket))
                        stop_streaming()
                        assert stream_control["streamcontrol"]["streamId"] == 2
                        assert speech_len > 2000
                        Logger.get().debug('complete read_noeou')
                        complete()
                        break

        if partial:
            spawn_all(_write, _read_noeou)
        else:
            spawn_all(_write, _read)

    connect_and_run_ioloop(server, _run, None, 25)


@gen.coroutine
def check_stored_context(group_id, dev_model, dev_manuf, voiceprints_count, check_modulo=False):
    Logger.get().debug('check_stored_context()')
    context_storage = ContextStorage(TEST_SESSION, group_id=group_id, dev_model=dev_model, dev_manuf=dev_manuf)
    yabio_context = yield context_storage.get_users_context()
    has_spotter_voiceprint = False
    has_request_voiceprint = False
    for user in yabio_context.users:
        Logger.get().debug('check_stored_context user_id={}'.format(user.user_id))
        has_spotter_voiceprint = False
        has_request_voiceprint = False
        vp_count = 0
        for voiceprint in user.voiceprints:
            Logger.get().debug('check_stored_context voiceprint source={}'.format(voiceprint.source))
            Logger.get().debug('check_stored_context voiceprint mds_url={}'.format(voiceprint.mds_url))
            Logger.get().debug('check_stored_context voiceprint request_id={}'.format(voiceprint.request_id))
            if voiceprint.source == 'spotter':
                has_spotter_voiceprint = True
            if voiceprint.source == 'request':
                has_request_voiceprint = True
            vp_count += 1
            assert voiceprint.mds_url.startswith('http') and '://' in voiceprint.mds_url
            assert voiceprint.timestamp
            assert voiceprint.text
            assert voiceprint.device_id
            assert voiceprint.device_model
            assert voiceprint.device_manufacturer

        s = sum([v.reg_num for v in user.voiceprints if v.source == 'request'])
        l = len([v for v in user.voiceprints if v.source == 'request']) - 1
        assert s == l*(l+1)/2
        if check_modulo:
            assert vp_count > 0 and vp_count % voiceprints_count == 0, (
                "expected modulos {} voiceprints per user, but have {}".format(voiceprints_count, vp_count))
        else:
            assert vp_count == voiceprints_count, \
                "expected {} voiceprints per user, but have {}".format(voiceprints_count, vp_count)
        if has_spotter_voiceprint and has_request_voiceprint:
            return  # SUCCESS

    if voiceprints_count:
        raise Exception('not found users with both voiceprints (spotter & request)')


def test_vins_bio(server, bio_group):
    start_time = time.time()
    test_user = _get_bass_test_user()

    if bio_group is not None:
        global BIOMETRY_GROUP
        BIOMETRY_GROUP = bio_group

    VOICE_INPUT["event"]["payload"]["biometry_group"] = BIOMETRY_GROUP
    VOICE_INPUT["event"]["payload"]["request"]["additional_options"]["oauth_token"] = test_user.token
    SYNC_STATE["event"]["payload"]["oauth_token"] = test_user.token
    SYNC_STATE["event"]["payload"]["vins"]["application"]["uuid"] = test_user.uuid
    SYNC_STATE["event"]["payload"]["uuid"] = test_user.uuid

    VOICE_INPUT["event"]["payload"]["advancedASROptions"]["utterance_silence"] = 120

    # delete current DB record (less dirty test)
    driver = IOLoop.current().run_sync(connect_ydb)
    accessor = YabioStorage(driver, get_table_name()).create_accessor(BIOMETRY_GROUP, DEV_MODEL, DEV_MANUFACTURER)
    IOLoop.current().run_sync(accessor.delete)
    IOLoop.current().run_sync(partial_tmpl(check_stored_context, BIOMETRY_GROUP, DEV_MODEL, DEV_MANUFACTURER, 0))  # user MUST not have voiceprints

    _test_voice_input(server, "tests/data/alice_break.opus", "хватит", "")
    _test_voice_input(server, "tests/data/bio0.opus", "давай познакомимся",  "постараюсь запомнить|голос будет связан с аккаунтом")
    _test_voice_input(server, "tests/data/bio5.opus", "меня зовут Вася", "мы двинемся дальше")
    _test_voice_input(server, "tests/data/bio1.opus", "я готов", "говорите своим нормальным голосом")

    for i in range(0, PHRASE_CNT - 1):
        Logger.get().info('learning {}'.format(i + 1))
        _test_voice_input(
            server,
            "tests/data/bio3.opus",
            "здравствуйте",
            "Скажите",
            spotter_file_name="tests/data/spotter.opus",
            spotter_text='алиса',
            partial=True,
        )
    Logger.get().info('learning {}'.format(PHRASE_CNT))

    _test_voice_input(server, "tests/data/bio3.opus", "здравствуйте", "Спасибо|Очень приятно|Рада познакомиться", partial=True)
    _test_voice_input(
        server,
        "tests/data/bio4.opus",
        "как меня зовут",
        "Вася",
        spotter_file_name="tests/data/spotter.opus",
        spotter_text='алиса'
    )

    _test_voice_input(
        server,
        "tests/data/whoami.opus",
        "как меня зовут",
        "Вася",
        spotter_file_name="tests/data/spotter.opus",
        spotter_text='алиса',
        partial=True,
    )
    _test_voice_input(
        server,
        "tests/data/whoami.opus",
        "как меня зовут",
        "Вася",
        spotter_file_name="tests/data/spotter.opus",
        spotter_text='алиса',
        partial=False,
    )
    _test_voice_input(
        server,
        "tests/data/whoami.opus",
        "как меня зовут",
        "Вася",
        partial=True,
    )
    _test_voice_input(
        server,
        "tests/data/whoami.opus",
        "как меня зовут",
        "Вася",
        partial=False,
    )

    time.sleep(5) # wait for all updated contexts
    # load context from ydb & validate (MUST containt spotter & request voiceprints)
    IOLoop.current().run_sync(partial_tmpl(check_stored_context, BIOMETRY_GROUP, DEV_MODEL, DEV_MANUFACTURER, (PHRASE_CNT-1)*2+1, check_modulo=True))  # user MUST have modulo 19 voiceprints

    if bio_group is None:  # it means that run manually
        _test_voice_input(
            server,
            "tests/data/forget_me.opus",
            "забудь мой голос",
            "Вася",
            spotter_file_name="tests/data/spotter.opus",
            spotter_text='алиса',
            partial=True,
        )

        _test_voice_input(
            server,
            "tests/data/yes_forget_me.opus",
            "да забудь мой голос",
            "больше не буду|выкинула из головы|Всё, забыла|мы можем познакомиться снова в любой момент",
            spotter_file_name="tests/data/spotter.opus",
            spotter_text='алиса',
            partial=True,
        )

        VOICE_INPUT["event"]["payload"]["vins_partial"] = True
        _test_voice_input(server, "tests/data/whoami.opus", "как меня зовут", "ваш голос мне не знак|не узнаю|не узна\+ю|запомни мое имя", partial=True)

    _free_bass_test_user(test_user.login)  # TODO: like this not work now

    Logger.get().info("Elapsed {}".format(time.time() - start_time))


def test_vins_bio_delete_unsupported_tags(server):
    start_time = time.time()
    test_user = _get_bass_test_user()
    VOICE_INPUT["event"]["payload"]["request"]["additional_options"]["oauth_token"] = test_user.token
    SYNC_STATE["event"]["payload"]["oauth_token"] = test_user.token
    SYNC_STATE["event"]["payload"]["vins"]["application"]["uuid"] = test_user.uuid
    SYNC_STATE["event"]["payload"]["uuid"] = test_user.uuid

    VOICE_INPUT["event"]["payload"]["advancedASROptions"]["utterance_silence"] = 120

    # create DB record with unsupported compatibility tags
    driver = IOLoop.current().run_sync(connect_ydb)
    accessor = YabioStorage(driver, get_table_name()).create_accessor(BIOMETRY_GROUP, DEV_MODEL, DEV_MANUFACTURER)
    IOLoop.current().run_sync(accessor.delete)
    yabio_ctx = ContextStorage(TEST_SESSION, group_id=BIOMETRY_GROUP, dev_model=DEV_MODEL, dev_manuf=DEV_MANUFACTURER)
    yabio_ctx.ydb_accessor = accessor
    yabio_ctx.yabio_context = YabioContext(
        group_id=BIOMETRY_GROUP,
        enrolling=[YabioVoiceprint(request_id='bad_request_id',
                                   compatibility_tag='bad_tag',
                                   format='ivector',
                                   source=s,
                                   mds_url='http://test_url',
                                   voiceprint=[1.0, 0.9, 0.8],
                                   )
                   for s in ['spotter', 'request']
                  ],
        )
    IOLoop.current().run_sync(partial_tmpl(yabio_ctx.add_user, 'test-user', ['bad_request_id'] ))
    del yabio_ctx

    IOLoop.current().run_sync(partial_tmpl(check_stored_context, BIOMETRY_GROUP, DEV_MODEL, DEV_MANUFACTURER, 2))

    _test_voice_input(server, "tests/data/alice_break.opus", "хватит", "")

    yabio_ctx = ContextStorage(TEST_SESSION, group_id=BIOMETRY_GROUP, dev_model=DEV_MODEL, dev_manuf=DEV_MANUFACTURER)
    yabio_ctx.ydb_accessor = accessor
    IOLoop.current().run_sync(partial_tmpl(yabio_ctx.load))

    for u in yabio_ctx.yabio_context.users:
        assert len(u.voiceprints) > 0
        for v in u.voiceprints:
            assert v.compatibility_tag != 'bad_tag'

    assert len(yabio_ctx.yabio_context.enrolling) > 0
    for e in yabio_ctx.yabio_context.enrolling:
        assert e.compatibility_tag != 'bad_tag'

    _free_bass_test_user(test_user.login)  # TODO: like this not work now
    IOLoop.current().run_sync(accessor.delete)  # tmp. replace _free_bass_test_user()

    Logger.get().info("Elapsed {}".format(time.time() - start_time))


def test_who_am_i(server, bio_group):
    start_time = time.time()
    test_user = _get_bass_test_user()

    if bio_group is None:
        pytest.skip('for manual running only')

    global BIOMETRY_GROUP
    BIOMETRY_GROUP = bio_group
    VOICE_INPUT["event"]["payload"]["biometry_group"] = BIOMETRY_GROUP
    VOICE_INPUT["event"]["payload"]["request"]["additional_options"]["oauth_token"] = test_user.token
    SYNC_STATE["event"]["payload"]["oauth_token"] = test_user.token
    SYNC_STATE["event"]["payload"]["vins"]["application"]["uuid"] = test_user.uuid
    SYNC_STATE["event"]["payload"]["uuid"] = test_user.uuid

    _test_voice_input(
        server,
        "tests/data/whoami.opus",
        "как меня зовут",
        "Вася|Петя",
        partial=True,
    )
    _test_voice_input(
        server,
        "tests/data/whoami.opus",
        "как меня зовут",
        "Вася|Петя",
        partial=False,
    )

    _free_bass_test_user(test_user.login)  # TODO: like this not work now
    Logger.get().info("Elapsed {}".format(time.time() - start_time))
