import json
import logging
import collections
import uuid as uuid_
import time

import tornado.ioloop
import tornado.httpserver
import tornado.web
import tornado.gen
import tornado.queues
import tornado.locks

from alice.uniproxy.library.global_counter import GlobalCounter, GlobalTimings

from alice.uniproxy.library.protos.uniproxy_pb2 import TSubwayMessage, TSubwayResponse, TSubwayMessageBatch

from alice.uniproxy.library.subway.common import SubwayRegistry
from alice.uniproxy.library.subway.pull_client.pull_client import ClientType

SUBWAY_PULL_BATCH_SIZE = 25


# ====================================================================================================================
class Subway(object):
    def __init__(self, nocache=False):
        super(Subway, self).__init__()
        self._log = logging.getLogger('subway.subway')
        self._processes = collections.defaultdict(self.create_queue)
        self._process_batch = collections.defaultdict(TSubwayMessageBatch)
        self._process_batch_wait = collections.defaultdict(tornado.locks.Condition)
        self._registry = SubwayRegistry()
        self._locks = collections.defaultdict(tornado.locks.Lock)
        self._nocache = nocache

    # ----------------------------------------------------------------------------------------------------------------
    def create_queue(self):
        return tornado.queues.Queue(3000)

    # ----------------------------------------------------------------------------------------------------------------
    def add(self, process_id, guid, ctype):
        if ctype == ClientType.GUID:
            guid = uuid_.UUID(guid).bytes
        self._registry.add(process_id, guid)

    # ----------------------------------------------------------------------------------------------------------------
    def remove(self, process_id, guid, ctype):
        if ctype == ClientType.GUID:
            guid = uuid_.UUID(guid).bytes
        self._registry.remove(process_id, guid)

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def fetch(self, process_id):
        try:
            MessagesCount = len(self._process_batch[process_id].Messages)
            if MessagesCount == 0:
                yield self._process_batch_wait[process_id].wait(timeout=time.time() + 60.0)

            MessagesCount = len(self._process_batch[process_id].Messages)
            if MessagesCount == 0:
                response = TSubwayMessageBatch()
                response.Status = 599
                response.Error = 'no messages'
            else:
                GlobalTimings.store('subway_queue_size', MessagesCount)
                response = self._process_batch[process_id]
                response.Status = 200
                self._process_batch[process_id] = TSubwayMessageBatch()
        except tornado.gen.TimeoutError:
            self._log.info('fetch process(%s) no messages received in 60 seconds')
            response.Status = 599
            response.Error = 'no messages'

        self._log.debug('fetch process(%s) fetched %d messages', process_id, MessagesCount)

        return response

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def push(self, guids, message):
        missing = []
        processes = set()

        for exists, guid, process_id in self._registry.enumerate_many(guids):
            if not exists:
                missing.append(guid)
                continue
            processes.add(process_id)

        for p in processes:
            self._log.debug('push to process %s', p)
            msg = self._process_batch[p].Messages.add()
            msg.CopyFrom(message)
            self._process_batch_wait[p].notify()

        return missing


# ====================================================================================================================
class ClientHandler(tornado.web.RequestHandler):
    def initialize(self, subway):
        self._subway = subway
        self._logger = logging.getLogger('subway.handler.client')

    # ----------------------------------------------------------------------------------------------------------------
    def _parse_request(self, body):
        data = json.loads(self.request.body.decode('utf-8'))
        guid = data.get('Guid')
        ctype = data.get('ClientType')
        return guid, ctype

    # ----------------------------------------------------------------------------------------------------------------
    def get(self):
        clients = self._subway._registry.enumerate_clients()
        self.finish(json.dumps(clients))

    # ----------------------------------------------------------------------------------------------------------------
    def post(self):
        try:
            process_id = self.request.headers.get('X-Process-Id')
            if process_id is None:
                self.finish({
                    'code':     400,
                    'error':    'expected X-Process-Id header'
                })
                return

            guid, ctype = self._parse_request(self.request.body)
            if guid is None:
                self.finish({
                    'code':     400,
                    'error':    'expected Guid in body'
                })
                return

            self._subway.add(process_id, guid, ctype)

            self.set_header('Content-Type', 'text/plain')
            self.finish('OK')
        except Exception as ex:
            self.set_status(500)
            self.set_header('Content-Type', 'text/plain')
            self.finish(str(ex))

    # ----------------------------------------------------------------------------------------------------------------
    def delete(self):
        try:
            process_id = self.request.headers.get('X-Process-Id')
            if process_id is None:
                self.set_status(400)
                self.finish('X-Process-Id header is expected')
                return

            guid, ctype = self._parse_request(self.request.body)
            self._subway.remove(process_id, guid, ctype)
            self.set_header('Content-Type', 'text/plain')
            self.finish('OK')
        except Exception as ex:
            self.set_status(500)
            self.finish(str(ex))


# ====================================================================================================================
class PullHandler(tornado.web.RequestHandler):
    def initialize(self, subway):
        self._subway = subway
        self._logger = logging.getLogger('subway.handler.pull')

    # ----------------------------------------------------------------------------------------------------------------
    def prepare(self):
        self._process_id = self.request.headers.get('X-Process-Id')

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def get(self):
        self.set_status(200)
        self.set_header('Content-Type', 'application/octet-stream')

        if self._process_id is None:
            response = TSubwayMessageBatch()
            response.Status = 400
            response.Error = 'expects X-Process-Id header'
            self.finish(response.SerializeToString())
            return

        response = yield self._subway.fetch(self._process_id)

        self.finish(response.SerializeToString())


# ====================================================================================================================
class PushHandler(tornado.web.RequestHandler):
    def initialize(self, subway):
        self._subway = subway
        self._logger = logging.getLogger('subway.handler.push')

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def post(self):
        GlobalCounter.MSSNGR_SUBWAY_RECV_SUMM.increment()

        content_type = self.request.headers.get('Content-Type')
        if content_type == 'application/json':
            yield self.post_json()
        elif content_type == 'application/octet-stream':
            yield self.post_protobuf()
        else:
            self.finish({
                'code':     400,
                'error':    'unsupported content-type "{}"'.format(content_type)
            })

    # ----------------------------------------------------------------------------------------------------------------
    #   old json format
    @tornado.gen.coroutine
    def post_json(self):
        GlobalCounter.MSSNGR_SUBWAY_FAIL_SUMM.increment()
        self.set_status(400)
        self.finish('invalid content-type, expects application/octet-stream with protobuf')

    # ----------------------------------------------------------------------------------------------------------------
    #   new protobuf format
    @tornado.gen.coroutine
    def post_protobuf(self):
        try:
            data = TSubwayMessage()
            data.ParseFromString(self.request.body)

            self._logger.debug('post_proto: iter destinations')

            GlobalTimings.store('subway_dest_count', len(data.Destinations))

            uid_mode = False
            guids = set()
            for dest in data.Destinations:
                if dest.Guid:
                    guids.add(dest.Guid)
                    self._logger.debug('add guid: {}'.format(dest.Guid))

            if not guids:
                uid_mode = True
                for dest in data.Destinations:
                    if dest.DeviceId:
                        guids.add(dest.DeviceId)
                        self._logger.debug('add device_id: {}'.format(dest.DeviceId))

            self._logger.debug('post_proto: pushing to queue')
            results = yield self._subway.push(list(guids), data)

            response = TSubwayResponse()
            response.Status = 200

            if uid_mode:
                for did in results:
                    response.MissingDevices.append(did)
            else:
                for guid in results:
                    response.MissingGuids.append(guid)
            response.Timestamp = int(time.time() * 1000000)

            self._logger.debug('post_proto: finishing')

            self.set_header('Content-Type', 'application/octet-stream')
            self.finish(response.SerializeToString())
        except Exception as ex:
            response = TSubwayResponse()
            response.Status = 500
            response.Message = str(ex)

            GlobalCounter.MSSNGR_SUBWAY_FAIL_SUMM.increment()
            self._logger.exception(ex)
            self.finish(response.SerializeToString())


# ====================================================================================================================
class StatHandler(tornado.web.RequestHandler):
    def initialize(self, subway):
        pass

    def get(self):
        method = self.get_argument('method', 'help')

        if method == 'help':
            pass
        else:
            pass
