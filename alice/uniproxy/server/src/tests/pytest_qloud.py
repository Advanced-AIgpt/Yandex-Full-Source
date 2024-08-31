#
# coding=utf
#
# в этом тесте не поднимаем локальный uniproxy а используем уже поднятый
# (по умолчанию qloud testing окружение)
# примеры:
#   py.test-3 -v pytest_qloud.py --qloud-env=stable
#   py.test-3 -v pytest_qloud.py
#

import json
import logging
import pytest
from tests.uniclient import process_test_session
from alice.uniproxy.library.settings import config
from .pytest_tests import COMMON_SESSION_FILES
from .pytest_tests import NEXT_UNIPROXY_SESSION_FILES
from .pytest_tests import NEXT_YALDI_SESSION_FILES
from .pytest_tests import NEXT_TTS_SESSION_FILES
from .pytest_tests import NEXT_VINS_SESSION_FILES
from .pytest_tests import ASR_TESTS
from .pytest_tests import impl_test_uniclient_asr


root_logger = logging.getLogger('')
file_logger = logging.StreamHandler()
file_logger.setFormatter(logging.Formatter('%(asctime)s %(levelname)-8s %(filename)s:%(lineno)d %(message)s'))
root_logger.addHandler(file_logger)
root_logger.setLevel(logging.DEBUG)


@pytest.mark.parametrize('script', COMMON_SESSION_FILES + NEXT_VINS_SESSION_FILES)
def test_uniclient(qloud_env, api_key, script):
    impl_tst(qloud_env, api_key, script)


@pytest.mark.parametrize('asr_test', ASR_TESTS)
def test_uniclient_asr(asr_test, qloud_env, api_key):
    uniproxy_url, api_key = get_url_and_key(qloud_env, api_key)
    impl_test_uniclient_asr(uniproxy_url, asr_test, api_key)


@pytest.mark.xfail
@pytest.mark.parametrize('script', NEXT_UNIPROXY_SESSION_FILES + NEXT_YALDI_SESSION_FILES + NEXT_TTS_SESSION_FILES)
def test_uniclient_nextgen(qloud_env, api_key, script):
    impl_tst(qloud_env, api_key, script)


def impl_tst(qloud_env, api_key, script):
    uniproxy_url, api_key = get_url_and_key(qloud_env, api_key)
    assert process_test_session(uniproxy_url, script, api_key) == []


def get_url_and_key(qloud_env, api_key):
    if qloud_env == 'testing' or qloud_env == 'test':
        uniproxy_url = 'wss://uniproxy.tst.voicetech.yandex.net/uni.ws'
        if api_key is None:
            api_key = config['key']
    elif qloud_env == 'stable':
        uniproxy_url = 'wss://voiceservices.yandex.net/uni.ws'
        if api_key is None:
            with open('../settings/production.json') as f:
                cfg = json.load(f)
            api_key = cfg['key']
    elif qloud_env == 'rtc':
        uniproxy_url = 'wss://uniproxy.alice.yandex.net/uni.ws'
        if api_key is None:
            with open('../settings/rtc_production.json') as f:
                cfg = json.load(f)
            api_key = cfg['key']
    elif qloud_env == 'station-stable':
        uniproxy_url = 'wss://voicestation.yandex.net/uni.ws'
        if api_key is None:
            with open('../settings/station.json') as f:
                cfg = json.load(f)
            api_key = cfg['key']
    elif qloud_env == 'prestable':
        uniproxy_url = 'wss://prestable.voiceservices.yandex.net/uni.ws'
        if api_key is None:
            with open('../settings/production.json') as f:
                cfg = json.load(f)
            api_key = cfg['key']
    elif qloud_env == 'priemka':
        uniproxy_url = 'wss://priemka.uniproxy.voicetech.yandex.net/uni.ws'
        if api_key is None:
            with open('../settings/production.json') as f:
                cfg = json.load(f)
            api_key = cfg['key']
    elif qloud_env == 'priemka-station':
        uniproxy_url = 'wss://priemka.uniproxy.voicetech.yandex.net/unistation.ws'
        if api_key is None:
            with open('../settings/station.json') as f:
                cfg = json.load(f)
            api_key = cfg['key']
    elif qloud_env == 'local':
        uniproxy_url = 'ws://localhost:8887/uni.ws'
        if api_key is None:
            api_key = config['key']
    elif qloud_env.startswith('ws:') or qloud_env.startswith('wss:'):
        uniproxy_url = qloud_env
        if api_key is None:
            api_key = config['key']
    else:
        err = 'unknown qloud type={}'.format(qloud_env)
        logging.error(err)
        raise Exception(err)

    return uniproxy_url, api_key
