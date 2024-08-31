import json
import logging
import pytest
import os
from alice.uniproxy.library.backends_asr import YaldiStream

from tests.basic import no_subway_server as uniproxy
from tests.uniclient import process_test_session
from alice.uniproxy.library.settings import config
from .pytest_tests import ASR_TESTS
from .pytest_tests import COMMON_SESSION_FILES
from .pytest_tests import NO_HEADERS_SESSION_FILES
from .pytest_tests import NEXT_UNIPROXY_SESSION_FILES
from .pytest_tests import NEXT_YALDI_SESSION_FILES
from .pytest_tests import NEXT_VINS_SESSION_FILES
from .pytest_tests import NEXT_TTS_SESSION_FILES
from .pytest_tests import impl_test_uniclient_asr


UNIPROXY_URL = 'ws://localhost:{}/uni.ws'.format(config['port'])
REQUIRED_MOCK_SCRIPTS = [
    'uniclient_yaldi_server_timeout.json',
    'uniclient_yaldi_client_timeout.json',
]


root_logger = logging.getLogger('')
file_logger = logging.StreamHandler()
file_logger.setFormatter(logging.Formatter('%(asctime)s %(levelname)-8s %(filename)s:%(lineno)d %(message)s'))
root_logger.addHandler(file_logger)
root_logger.setLevel(logging.DEBUG)


@pytest.mark.parametrize('script', COMMON_SESSION_FILES + NEXT_UNIPROXY_SESSION_FILES + NEXT_VINS_SESSION_FILES + NEXT_TTS_SESSION_FILES + NO_HEADERS_SESSION_FILES)
def test_uniclient(uniproxy, script):
    assert process_test_session(UNIPROXY_URL, script, config['key']) == []


@pytest.mark.parametrize('asr_test', ASR_TESTS)
def test_uniclient_asr(uniproxy, asr_test):
    impl_test_uniclient_asr(UNIPROXY_URL, asr_test)


@pytest.mark.parametrize(
    'script',
    NEXT_YALDI_SESSION_FILES + REQUIRED_MOCK_SCRIPTS,
)
def test_backend_yaldi_uniclient(uniproxy, script):
    """ yaldi backend specific tests
    """
    if script == 'uniclient_yaldi_server_timeout.json':
        # temporary (mokey)patching config (cause yaldi server timeout error)
        old_timeout = config['asr']['yaldi_inactivity_timeout']
        try:
            config.set_by_path('asr.yaldi_inactivity_timeout', 2)
            assert process_test_session(UNIPROXY_URL, script, config['key']) == []
        finally:
            config.set_by_path('asr.yaldi_inactivity_timeout', old_timeout)
        return
    if script == 'uniclient_yaldi_client_timeout.json':
        old_timeout = config['asr']['client_inactivity_timeout']
        try:
            config.set_by_path('asr.client_inactivity_timeout', 2)
            assert process_test_session(UNIPROXY_URL, script, config['key']) == []
        finally:
            config.set_by_path('asr.client_inactivity_timeout', old_timeout)
        return

    assert process_test_session(UNIPROXY_URL, script, config['key']) == []


def test_backend_yaldi_fallback_dialogeneralfast(uniproxy, monkeypatch):
    old_read_init_response = YaldiStream.read_init_response
    switch_to_fallback = []

    def mock_read_init_response(self, send_result):
        if (self.yaldi_host, 'dialogeneralfast') in self.fallback_topics:
            logging.debug('### MOCK-ing imitation fail yaldi server')
            switch_to_fallback.append('failfirst')
            old_read_init_response(self, None)
        else:
            if 'dialogeneralfast' in self.yaldi_proto.uri:
                switch_to_fallback.append('usefast')
            old_read_init_response(self, send_result)
    monkeypatch.setattr(YaldiStream, 'read_init_response', mock_read_init_response)
    assert process_test_session(UNIPROXY_URL, 'uniclient_asr_ru_dialogeneral.json', config['key']) == []
    assert 'failfirst' in switch_to_fallback and 'usefast' in switch_to_fallback
