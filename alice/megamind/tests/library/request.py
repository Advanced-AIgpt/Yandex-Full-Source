# -*- coding: utf-8 -*-

from __future__ import unicode_literals

import abc
import collections
import copy
import datetime
import html
import json
import logging
import pytz
import requests
import time
import uuid
import yatest.common

logger = logging.getLogger(__name__)

REQUEST = {
    "application": {
        "app_id": "ru.yandex.searchplugin.dev",
        "app_version": "8.20",
        "client_time": "20190620T204423",
        "device_id": "10f62b3ef008b2d9ff2494d0f25f3868",
        "device_manufacturer": "Xiaomi",
        "device_model": "Redmi Note 4",
        "lang": "ru-RU",
        "os_version": "7.1.2",
        "platform": "android",
        "timestamp": "1561052663",
        "timezone": "Europe/Moscow",
        "uuid": "d538c1b27cc64771bb895f075817d752"
    },
    "header": {
        "prev_req_id": "533f6ea6-787d-4d41-a97b-88e07bfaa67c",
        "request_id": "3581cfd8-70e9-49a5-b174-4865ce5abd42",
        "sequence_number": 1190
    },
    "request": {
        "additional_options": {
            "bass_options": {
                "client_ip": "2a02:6b8:0:402:8d2a:50c0:dd3c:62c2",
                "cookies": [],
                "filtration_level": 1,
                "screen_scale_factor": 3,
                "user_agent":
                    "Mozilla/5.0 (Linux; Android 7.1.2; Redmi Note 4 Build/NJH47F; wv) AppleWebKit/537.36 (KHTML, like Gecko) Version/4.0 Chrome/66.0.3359.139 Mobile Safari/537.36 yandexSearch/8.20"
            },
            "divkit_version": "2.0.1",
            "yandex_uid": "37432040"
        },
        "device_state": {
            "sound_level": 6,
            "sound_muted": False
        },
        "experiments": {
        },
        "laas_region": {
            "city_id": 213,
            "country_id_by_ip": 225,
            "is_anonymous_vpn": False,
            "is_gdpr": False,
            "is_hosting": False,
            "is_mobile": False,
            "is_public_proxy": False,
            "is_serp_trusted_net": False,
            "is_tor": False,
            "is_user_choice": False,
            "is_yandex_net": True,
            "is_yandex_staff": True,
            "latitude": 55.753215,
            "location_accuracy": 15000,
            "location_unixtime": 1561052656,
            "longitude": 37.622504,
            "precision": 2,
            "probable_regions": [],
            "probable_regions_reliability": 1,
            "region_by_ip": 213,
            "region_home": 0,
            "region_id": 213,
            "should_update_cookie": False,
            "suspected_latitude": 55.753215,
            "suspected_location_accuracy": 15000,
            "suspected_location_unixtime": 1561052656,
            "suspected_longitude": 37.622504,
            "suspected_precision": 2,
            "suspected_region_city": 213,
            "suspected_region_id": 213
        },
        "location": {
            "accuracy": 282.0300598,
            "lat": 55.63529205,
            "lon": 37.60366058,
            "recency": 168975153
        },
        "reset_session": False,
        "voice_session": True
    },
    "__test_data__": {}
}


class TestBuilder(object):
    def __init__(self, name, *args):
        self.appliers = args
        self.name = name

    def apply(self, request_builder):
        for applier in self.appliers:
            applier.apply(request_builder)
        return request_builder

    @staticmethod
    def idsfn(self):
        """
        This function is needed for pytest.mark.parametrize decorator to return
        a valid name for tests made of the TestBulder class.
        Self is really self despite the fact that it is staticmethod.
        """
        return self.name


class Apply:
    """
    Base class for all request builder such as cgis, headers, body amendmends, etc...
    """
    @abc.abstractmethod
    def apply(self, request_builder):
        pass


class Event(Apply):
    """
    Base class for speechkit request event builder.
    """
    @abc.abstractmethod
    def build(self):
        pass

    def apply(self, request_builder):
        request_builder.json_body()['request']['event'] = self.build()


class ServerActionEvent(Event):
    def __init__(self, name, payload):
        self.name = name
        self.payload = payload

    def build(self):
        event = {
            'name': self.name,
            'payload': self.payload,
            'type': 'server_action'
        }
        return event


class ImageInputEvent(Event):
    def __init__(self, image_url):
        self.image_url = image_url

    def build(self):
        event = {
            'name': '',
            'payload': {
                'capture_mode': 'photo',
                'img_url': self.image_url
            },
            'type': 'image_input'
        }
        return event


class TextInputEvent(Event):
    def __init__(self, text):
        self.text = text

    def build(self):
        event = {
            'name': '',
            'text': self.text,
            'type': 'text_input'
        }
        return event


class VoiceEvent(Event):
    def __init__(self, utterance, words):
        self.utterance = utterance
        self.words = words

    def build(self):
        event = {
            'asr_result': [
                {
                    'confidence': 1,
                    'utterance': str(self.utterance),
                    'words': self.words
                }
            ],
            'end_of_utterance': True,
            'name': '',
            'type': 'voice_input'
        }
        return event


class SimpleVoiceEvent(VoiceEvent):
    def __init__(self, utterance, words):
        words_struct = []
        for word in words:
            words_struct.append({'confidence': 1, 'value': str(word)})
        super().__init__(utterance, words_struct)


class VerySimpleVoiceEvent(SimpleVoiceEvent):
    def __init__(self, utterance):
        super().__init__(utterance, list(filter(lambda word: len(word), utterance.split(' '))))


class AddCGI(Apply):
    def __init__(self, name, value):
        self.name = name
        self.value = value

    def apply(self, request_builder):
        request_builder.add_cgi(self.name, self.value)


class EnableExpFlag(Apply):
    def __init__(self, name):
        self.name = name

    def apply(self, request_builder):
        request_builder.enable_expflag(self.name)


class DisableExpFlag(Apply):
    def __init__(self, name):
        self.name = name

    def apply(self, request_builder):
        request_builder.disable_expflag(self.name)


class SetPath(Apply):
    def __init__(self, path):
        self.path = path

    def apply(self, request_builder):
        request_builder.set_path(self.path)


class RequestBuilder(object):
    def __init__(self):
        self.path = 'speechkit/app/pa/'
        self.data = copy.deepcopy(REQUEST)
        self.headers = []
        self.cgis = []

    def json_body(self):
        return self.data

    def enable_expflag(self, flag_name):
        self.json_body()['request']['experiments'][flag_name] = '1'
        return self

    def disable_expflag(self, flag_name):
        self.json_body()['request']['experiments'].pop(flag_name)
        return self

    def add_cgi(self, name, value):
        self.cgis.append([name, value])
        return self

    def cgi_string(self):
        result = ''
        for param in self.cgis:
            result += '&' if len(result) else '?'
            result += html.escape(param[0]) + '=' + html.escape(param[1])
        return result

    def add_header(self, name, value):
        self.headers.append([name, value])

    def add_headers(self, headers):
        self.headers += headers

    def set_path(self, path):
        self.path = path


class Response:
    def __init__(self, test_name, response):
        self.filename = '{}.response'.format(test_name)
        self.resp = response

    def response(self):
        return self.resp

    def ya_canonical(self, response_changer=None):
        content = self.resp.content
        if response_changer:
            for changer in response_changer:
                content = changer.modify(content)
        with open(self.filename, 'wb') as f:
            f.write('{}\n{}\n'.format(self.resp.status_code, collections.OrderedDict(self.resp.headers)).encode('utf-8'))
            if isinstance(content, bytes):
                f.write(content)
            else:
                f.write(content.encode('utf-8'))
        return yatest.common.canonical_file(self.filename)


class SpeechKit:
    def __init__(self, test_location, session, megamind, joker):
        """
        test_location is a pytest location tuple (filesystempath, lineno, domaininfo)
                      (https://docs.pytest.org/en/latest/reference.html#_pytest.runner.TestReport.location)
        """
        self.megamind = megamind
        self.session = session
        self.joker = joker
        location = test_location[0].replace('/', '-')
        test = test_location[2].replace('=', '~').replace('::', '@').replace(':', '@').replace('/', '-')
        self.test_name = location + '@' + test
        try:
            test_data = session.get_metadata(self.test_name)
        except Exception as e:
            test_data = {'epoch': int(time.time()), 'version': uuid.uuid4().hex}
            session.set_metadata(self.test_name, test_data)
        self.test_data = test_data

    def run_test_request(self, test_req_builder):
        # Sync stubs for the given request.
        self.session.sync_stubs(self.joker, self.test_name)

        # RequestBuilder must be there!
        request = None

        if type(test_req_builder) is TestBuilder:
            request = RequestBuilder()
            test_req_builder.apply(request)
        elif type(test_req_builder) is RequestBuilder:
            request = test_req_builder
        else:
            raise Exception('unsupported test request builder %s', test_req_builder)

        headers = {
            'content-type': 'application/json; charset=utf-8',
        }
        self.joker.insert_headers(self.test_name, headers)
        request.add_headers(headers)

        json_body = json.dumps(self._build(request.json_body()), ensure_ascii=False)
        resp = requests.post(self.megamind.make_url(request.path) + request.cgi_string(), data=json_body.encode('utf-8'), headers=headers)
        return Response(self.test_name, resp)

    def _build(self, json_body):
        epoch = self.test_data['epoch']
        tz = pytz.timezone(json_body['application']['timezone'])
        utc_dt = pytz.utc.localize(datetime.datetime.utcfromtimestamp(epoch))
        json_body['application']['timestamp'] = str(epoch)
        json_body['application']['client_time'] = utc_dt.astimezone(tz).strftime('%Y%m%dT%H%M%S')
        json_body['__test_data__']['version'] = self.test_data['version']
        return json_body
