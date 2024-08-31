import hashlib
import json
import logging
import os
import time
import uuid as uuid_
from enum import IntEnum

import tornado.gen

from alice.uniproxy.library.async_http_client import QueuedHTTPClient, HTTPRequest, HTTPError
from alice.uniproxy.library.global_counter import GlobalCounter
from alice.uniproxy.library.global_counter import UnistatTiming
from alice.uniproxy.library.messenger.client_locator import ClientLocator
from alice.uniproxy.library.messenger.msgsettings import UNIPROXY_MESSENGER_JSON_CONTENT_FIELDS
from alice.uniproxy.library.protos.uniproxy_pb2 import TSubwayMessageBatch
from alice.uniproxy.library.utils.proto_to_json import MessageToDict2
from alice.uniproxy.library.subway.pull_client.registry import registry_instance
from alice.uniproxy.library.notificator_api.locator import register_device


class ClientType(IntEnum):
    DEVICE = 1
    GUID = 2


# ====================================================================================================================
class PullClient(object):
    class UniSystemId:  # can be used instead real UniSystem instance for remove_client() method
        def __init__(self, session_id, guid, ctype):
            self.session_id = session_id
            self.subway_client_type = ctype
            self.subway_uid = None
            if ctype == ClientType.DEVICE:
                self.subway_uid = guid
            elif ctype == ClientType.GUID:
                self.subway_uid = str(uuid_.UUID(bytes=guid))

    def __init__(self, port=7777, nocache=False, wait=True):
        super(PullClient, self).__init__()
        self._log = logging.getLogger('subway.client.pull')
        self._registry = registry_instance()
        self._host = 'localhost'
        self._port = port
        self._url_pull = '/pull'
        self._url_client = '/client'
        self._url_ping = '/ping'
        self._url_ping_sync = 'http://localhost:{}/ping'.format(self._port)
        self._client = None
        self._nocache = nocache
        self._wait = wait

    # ----------------------------------------------------------------------------------------------------------------
    def port(self) -> int:
        return self._port

    # ----------------------------------------------------------------------------------------------------------------
    def _create_proceess_id(self):
        return hashlib.md5(str(os.getpid()).encode('utf-8')).hexdigest()

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def add_client(self, unisystem, **kwargs):
        client_id = unisystem.session_id
        guid = kwargs.get('guid', unisystem.subway_uid)
        ctype = unisystem.subway_client_type
        device_model = unisystem.device_model
        supported_features = unisystem.session_data.get('supported_features', None)

        self._registry.add(unisystem, guid, client_id)

        request = HTTPRequest(
            self._url_client,
            method='POST',
            headers={
                'X-Process-Id': self._create_proceess_id(),
                'Content-Type': 'application/json;charset=utf-8'
            },
            body=json.dumps({
                'ClientId': client_id,
                'Guid':     guid,
                'ClientType': ctype,
            }).encode('utf-8')
        )

        try:
            response = yield self._client.fetch(request)

            if response.code != 200:
                message = response.body.decode('utf-8')
                self._log.error('CODE %d: %s', response.code, message)
                return False, message

            if not self._nocache:
                if ctype == ClientType.DEVICE:
                    yield register_device(
                        unisystem.srcrwr['NOTIFICATOR'],
                        unisystem.puid,
                        guid,
                        device_model,
                        supported_features
                    )
                elif ctype == ClientType.GUID:
                    locator = ClientLocator()
                    yield tornado.gen.multi(locator.update_location(guid, client_id))
                else:
                    self._log.error(f'unknown client type {ctype}')
                    return False, f'unknown client type {ctype}'

            return True, 'ok'
        except HTTPError as ex:
            return False, str(ex)

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def sync_client(self, unisystem):
        pass

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def remove_client(self, unisystem):
        try:
            client_id = unisystem.session_id
            guid = unisystem.subway_uid
            ctype = unisystem.subway_client_type
        except ReferenceError:
            return False, 'remove_client() called for empty weakref.proxy(unisystem)'

        self._registry.remove(guid, client_id)

        request = HTTPRequest(
            self._url_client,
            method='DELETE',
            headers={
                'X-Process-Id': self._create_proceess_id(),
                'Content-Type': 'application/json;charset=utf-8',
            },
            body=json.dumps({
                'ClientId': client_id,
                'Guid':     guid,
                'ClientType': ctype,
            }).encode('utf-8')
        )

        try:
            response = yield self._client.fetch(request)

            if response.code != 200:
                message = response.body.decode('utf-8')
                self._log.error('CODE %d: %s', response.code, message)
                return False, message

            return True, 'ok'
        except HTTPError as ex:
            return False, str(ex)

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def enumerate(self):
        try:
            request = HTTPRequest(
                self._url_client,
                method='GET',
                headers={
                    'X-Process-Id': self._create_proceess_id(),
                },
                request_timeout=60.0
            )
            response = yield self._client.fetch(request)
            return json.loads(response.body.decode('utf-8'))
        except Exception as ex:
            self._log.exception(ex)
            return []

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def pull(self, timeout=60.0):
        try:
            request = HTTPRequest(
                self._url_pull,
                method='GET',
                headers={
                    'X-Process-Id': self._create_proceess_id(),
                },
                request_timeout=timeout
            )

            response = yield self._client.fetch(request)

            self._log.debug('proto response parsing...')
            proto = TSubwayMessageBatch()
            proto.ParseFromString(response.body)

            code = proto.Status
            self._log.debug('proto response status=%d', proto.Status)
            if code != 200:
                message = proto.Error
                return False, message

            return True, proto.Messages
        except HTTPError as ex:
            self._log.error('X-Process-Id(%s) => %s', self._create_proceess_id(), ex)
            return False, str(ex)
        except Exception as ex:
            self._log.exception(ex)
            return False, str(ex)

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def _delivery_loop(self, messages):
        try:
            with UnistatTiming('subway_delivery_time'):
                for proto in messages:
                    ctype = None
                    if proto.Destinations[0].DeviceId:
                        ctype = ClientType.DEVICE
                    else:
                        ctype = ClientType.GUID

                    if ctype == ClientType.DEVICE:
                        data = proto.QuasarMsg.SerializeToString()
                    else:
                        data = MessageToDict2(
                            proto.MessengerMsg,
                            json_fields=UNIPROXY_MESSENGER_JSON_CONTENT_FIELDS,
                            skip_fields=['Token', 'Guids']
                        )

                    for dest in proto.Destinations:
                        id_ = dest.Guid or dest.DeviceId
                        for guid, session_id, uniws in self._registry.enumerate(id_):
                            if uniws is None:
                                self.schedule_remove_entry(guid, session_id, ctype)
                                continue

                            try:
                                uniws.on_subway_message(data)
                                GlobalCounter.MSSNGR_SUBWAY_SENT_SUMM.increment()
                            except ReferenceError:
                                GlobalCounter.MSSNGR_SUBWAY_CONNECTION_DROPPED_SUMM.increment()
                                self.schedule_remove_entry(guid, session_id, ctype)
                            except Exception as ex:
                                self._log.error('process_event error: %s', ex)
                                GlobalCounter.MSSNGR_SUBWAY_UNISYSTEM_FAIL_SUMM.increment()
        except Exception as ex:
            self._log.error('delivery loop fail: %s', ex)

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def _pull_loop(self):
        yield self.wait_for_ready()

        try:
            while True:
                self._log.debug('X-Process-Id(%s) -> pulling...', self._create_proceess_id())
                ok, messages = yield self.pull(timeout=3000.0)
                if not ok:
                    continue

                tornado.ioloop.IOLoop.current().spawn_callback(self._delivery_loop, messages)
        except Exception as ex:
            self._log.error('pull_loop error: %s', ex)
            GlobalCounter.MSSNGR_SUBWAY_PULL_FAIL_SUMM.increment()
            tornado.ioloop.IOLoop.current().spawn_callback(self._pull_loop)

    # ----------------------------------------------------------------------------------------------------------------
    def schedule_remove_entry(self, guid, session_id, ctype):
        tornado.ioloop.IOLoop.current().spawn_callback(
            self.remove_client,
            self.UniSystemId(session_id, guid, ctype),
        )

    # ----------------------------------------------------------------------------------------------------------------
    def start(self, start_worker=True, pool_size=26):
        self._client = QueuedHTTPClient(host=self._host, port=self._port, queue_size=200, pool_size=pool_size)
        if start_worker:
            tornado.ioloop.IOLoop.current().spawn_callback(self._pull_loop)
            tornado.ioloop.IOLoop.current().spawn_callback(self._pull_loop)
            tornado.ioloop.IOLoop.current().spawn_callback(self._pull_loop)
            tornado.ioloop.IOLoop.current().spawn_callback(self._pull_loop)

    @tornado.gen.coroutine
    def stop(self):
        if self._client:
            yield self._client.stop()

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def wait_for_ready(self, limit=3600.0):
        if not self._wait:
            return

        self._log.info('waiting for subway')
        status = 0
        ts = time.time()
        while status != 200:
            try:
                yield tornado.gen.sleep(0.25)

                request = HTTPRequest(
                    self._url_ping,
                    method='GET'
                )

                response = yield self._client.fetch(request)

                status = response.code
            except Exception:
                status = 0

            yield tornado.gen.sleep(0.1)

            if time.time() - ts > limit:
                break

    # ----------------------------------------------------------------------------------------------------------------
    def wait_for_ready_sync(self):
        if not self._wait:
            return

        client = tornado.httpclient.HTTPClient()

        self._log.info('waiting for subway')

        status = 0
        while status != 200:
            try:
                request = tornado.httpclient.HTTPRequest(
                    self._url_ping_sync,
                    method='GET',
                    connect_timeout=0.5
                )

                response = client.fetch(request)
                status = response.code
            except tornado.httpclient.HTTPError as ex:
                status = ex.code
            except Exception:
                status = 0

            time.sleep(0.1)
