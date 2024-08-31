#!/usr/bin/env python
# encoding: utf-8
import json
import uuid
import datetime

import pytest
from tornado.websocket import websocket_connect

from alice.uniproxy.library.settings import config
from tests.basic import async_test, server
from tornado import gen


def assert_with_json_context(condition, json_context):
    if condition:
        return
    print('condition failed for:\n', json.dumps(json_context, indent=4, ensure_ascii=False))
    assert False


class TestHelpers:
    @staticmethod
    def is_vins_response(message):
        return message["directive"]["header"]["name"] == "VinsResponse"

    @staticmethod
    def get_vins_response(message):
        assert_with_json_context(TestHelpers.is_vins_response(message), message)
        return message["directive"]["payload"]["response"]

    @staticmethod
    def is_general_conversation(vins_response):
        for m in vins_response.get('meta', []):
            if m['type'] == 'external_skill' and m['skill_name'] == 'Чат с Алисой' and m['deactivating'] is False:
                return True
        return False

    @staticmethod
    def deactivating_external_skill(vins_response):
        for m in vins_response.get('meta', []):
            if m['type'] == 'external_skill' and m['deactivating'] is True:
                return True
        return False

    @staticmethod
    def has_external_skills(vins_response):
        for m in vins_response.get('meta', []):
            if m['type'] == 'external_skill':
                return True
        return False


class UniproxyTestClient:
    def __init__(self, server_host_and_port, message_timeout_seconds):
        self._server_host_and_port = server_host_and_port
        self._message_timeout_seconds = message_timeout_seconds
        self._uuid = "f" * 16 + str(uuid.uuid4()).replace("-", "")[16:32]
        self._socket = None
        self._experiments = []
        self._last_request_id = None

    async def start(self):
        self._socket = await websocket_connect("ws://{0}/uni.ws".format(self._server_host_and_port))

    def close(self):
        if self._socket:
            self._socket.close()
            self._socket = None

    async def vins_query_text(self, text):
        await self._send_text_input(text)
        response = await self.read_message()
        assert_with_json_context(TestHelpers.is_vins_response(response), response)
        return TestHelpers.get_vins_response(response)

    async def read_message(self):
        message = await gen.with_timeout(datetime.timedelta(seconds=self._message_timeout_seconds),
                                         self._get_socket().read_message())
        if message is None:
            return None
        return message if message is bytes else json.loads(message)

    @property
    def uuid(self):
        return self._uuid

    def set_experiments(self, new_experiments):
        self._experiments = new_experiments

    async def send_sync_state(self):
        message = {
                "event": {
                    "header": {
                        "namespace": "System",
                        "name": "SynchronizeState",
                        "messageId": str(uuid.uuid4()),
                    },
                    "payload": {
                        "auth_token": config["key"],
                        "uuid": self._uuid,
                        "vins": {
                            "application": {
                                "app_id": "uniproxy.test",
                                "device_manufacturer": "Yandex",
                                "app_version": "1.2.3",
                                "os_version": "6.0.1",
                                "device_model": "Station",
                                "platform": "android",
                                "uuid": self._uuid,
                                "lang": "ru-RU",
                            },
                        }
                    }
                }
            }
        await self._send_message(message)

    async def _send_text_input(self, text):
        request_id = str(uuid.uuid4())
        prev_req_id = self._last_request_id
        self._last_request_id = request_id
        message = {
            "event": {
                "header": {
                    "namespace": "VINS",
                    "name": "TextInput",
                    "messageId": str(uuid.uuid4())
                },
                "payload": {
                    "header": {
                        "request_id": request_id,
                        "prev_req_id": prev_req_id
                    },
                    "request": {
                        "event": {
                            "type": "text_input",
                            "text": text
                        },
                        "experiments": self._experiments
                    },
                    "application": {
                        "client_time": datetime.datetime.now().strftime("%Y%m%dT%H%M%S"),
                        "timezone": "Europe/Moscow",
                        "timestamp": "%d" % (datetime.datetime.now().timestamp())
                    },
                    "lang": "ru-RU",
                    "topic": "desktopgeneral",
                    "vins_partial": True
                }
            }
        }
        await self._send_message(message)

    async def _send_message(self, message):
        await gen.with_timeout(datetime.timedelta(seconds=self._message_timeout_seconds),
                               self._get_socket().write_message(json.dumps(message)))

    def _get_socket(self):
        if not self._socket:
            raise RuntimeError('socket is not connected')
        return self._socket


@pytest.fixture()
def uniproxy_client(server):
    client = UniproxyTestClient('localhost:' + str(config['port']), 5)
    yield client
    client.close()


@async_test
async def test_wait_for_previous_query_session_to_save_before_processing_new_one(server, uniproxy_client):
    print('test uuid [{0}]'.format(uniproxy_client.uuid))

    await uniproxy_client.start()
    await uniproxy_client.send_sync_state()

    vins_response = await uniproxy_client.vins_query_text("Алиса, давай поболтаем!")
    assert_with_json_context(TestHelpers.is_general_conversation(vins_response), vins_response)

    vins_response = await uniproxy_client.vins_query_text("Хватит болтать")
    assert_with_json_context(TestHelpers.deactivating_external_skill(vins_response), vins_response)

    vins_response = await uniproxy_client.vins_query_text("Какая погода в Москве?")
    assert_with_json_context(not TestHelpers.has_external_skills(vins_response), vins_response)
