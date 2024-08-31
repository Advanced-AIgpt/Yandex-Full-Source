import base64
import enum
import json
import logging
from dataclasses import dataclass

import cyson

from .event import ImageInput
from .serialization import deserialize, serialize
from alice.acceptance.modules.request_generator.scrapper.lib.api import response as api_response
from alice.acceptance.modules.request_generator.scrapper.lib.api.run import set_error_response
from alice.acceptance.modules.request_generator.scrapper.lib.api.run import set_response


class EmptyFileException(Exception):
    pass


class MessageType(enum.Enum):
    EVO_TESTS_STARTED = 'evo_tests_started'
    TEST_RESULT = 'test_result'
    UNIPROXY_SETTINGS = 'uniproxy_settings'
    REQUEST = 'request'


class ResponseType(enum.Enum):
    """Possible values of the 'Type' field in response json"""

    VINS = b'VinsResponse'
    TTS_STREAM = b'TtsSpeechResponse'


class RequestType(enum.Enum):
    """Used to specify what request is going from evo"""

    TEXT = 'text'
    VOICE = 'tts_stream'
    IMAGE_INPUT = 'image_input'
    SERVER_ACTION = 'server_action'


def _get_response_by_type(response, response_type):
    """It's supposed that response.Type can take values 'Asr', 'Vins' 'TtsText' or 'TtsStream'"""
    return response.get(response_type.value, '')


class UniproxyClientParams(object):
    def __init__(self, settings, application):
        self.AuthToken = settings.auth_token
        self.OAuthToken = settings.oauth_token
        self.DisableLocalExperiments = False
        self.SyncStateExperiments = serialize(settings.experiments)
        self.ShootingSource = 'evo_tests'

        self.Application = serialize(application)
        self.Uuid = application.Uuid
        self.Lang = application.Lang
        self.Timezone = application.Timezone

        self.SupportedFeatures = serialize(settings.sync_state_supported_features)


@dataclass
class ExtraRequestParams(object):
    RequestId: str
    Payload: str
    FetcherMode: str
    Text: str
    Format: str
    Topic: str
    VoiceData: str

    def __init__(self, request_id, payload):
        self.RequestId = request_id
        self.Payload = payload


class ScraperMessage(object):
    def __init__(self, message_type, message):
        self.message_type = message_type.value
        self.message = message


class TestResult(object):
    def __init__(self, report):
        self.test_status = report.outcome
        self.location = f'{report.location[0]}::{report.location[2]}'
        if report.failed:
            self.failed_log = report.longreprtext


class ScraperClient(object):
    _input = None
    _output = None
    is_evo_tests_started = False

    @classmethod
    def open_files(cls, input_fifo, output_fifo):
        print(f'Input fifo: {input_fifo}. Output fifo: {output_fifo}')
        if not cls._input:
            cls._input = open(input_fifo, mode='rb')
        if not cls._output:
            cls._output = open(output_fifo, mode='wb')

    @classmethod
    def send_start(cls):
        if not cls.is_evo_tests_started:
            set_response(api_response.AliceEvoTestsStarted(), cls._output)
            cls.is_evo_tests_started = True

    @classmethod
    def send_uniproxy_settings(cls, params):
        dict_params = json.loads(serialize(params))
        set_response(api_response.AliceUniproxySettings(dict_params), cls._output)

    @classmethod
    def send_request(cls, request_type, request):
        request.FetcherMode = request_type.value
        dict_request = json.loads(serialize(request))
        set_response(api_response.AliceNextRequest(dict_request), cls._output)
        return cls._get_response()

    @classmethod
    def send_test_result(cls, report):
        report = TestResult(report)
        dict_report = json.loads(serialize(report))
        set_response(api_response.AliceEvoTestResult(dict_report), cls._output)

    @classmethod
    def send_end_session(cls):
        set_response(api_response.AliceEndSessionOk(), cls._output)

    @classmethod
    def _read_input(cls):
        line = cls._input.readline()
        if not line:
            raise EmptyFileException(f'Cannot read answer from {cls._input.name}')
        return line

    @classmethod
    def _get_response(cls):
        logging.info(f'Trying to read response from {cls._input.name}')
        try:
            raw_request = cls._read_input()
            parsed_yson = cyson.loads(raw_request.strip())
            payload = parsed_yson[b'payload']
            logging.info(f'Response read successfully: {raw_request}')
            return payload
        except KeyError as e:
            message = f'Bad request format, required field missed "{e}"'
            set_error_response(message, cls._output)
        except Exception as e:
            message = f'Exception in "ScraperClient._get_response": {e}'
            set_error_response(message, cls._output)


class ScraperUniclient(object):
    def __init__(self, settings, application):
        self._asr_topic = settings.asr_topic
        ScraperClient.send_uniproxy_settings(UniproxyClientParams(settings, application))

    def request(self, payload, request=None):
        logging.info(f'RequestId: {payload.header.RequestId}')
        logging.info(f'Setrace URL: https://setrace.yandex-team.ru/alice/sessions/{payload.header.RequestId}')
        logging.info(f'Send request: {request}')
        logging.info(f'Send payload: {serialize(payload)}')

        responses = self._send_request(payload, request)
        tts_response = _get_response_by_type(responses, ResponseType.TTS_STREAM)
        vins_response = deserialize(_get_response_by_type(responses, ResponseType.VINS))

        logging.info(f'Receive: {vins_response}')
        return vins_response, tts_response

    def _send_request(self, payload, request):
        params = ExtraRequestParams(payload.header.RequestId, serialize(payload))
        if not request:
            if isinstance(payload.request.get('event'), ImageInput):
                return self._send_image_input(params)
            else:
                return self._send_server_action(params)
        if isinstance(request, str):
            return self._send_text_request(params, request)
        if isinstance(request, bytes):
            return self._send_voice_request(params, request)

    def _send_text_request(self, request, text):
        request.Text = text
        return ScraperClient.send_request(RequestType.TEXT, request)

    def _send_image_input(self, request):
        return ScraperClient.send_request(RequestType.IMAGE_INPUT, request)

    def _send_server_action(self, request):
        return ScraperClient.send_request(RequestType.SERVER_ACTION, request)

    def _send_voice_request(self, request, voice_data):
        request.Format = 'audio/opus'
        request.Topic = self._asr_topic
        request.VoiceData = base64.b64encode(voice_data).decode('utf-8')
        return ScraperClient.send_request(RequestType.VOICE, request)
