import uuid as uuid_
import json
import struct
import time
import logging

import tornado.web

try:
    from cityhash import hash64 as CityHash64
except:
    from clickhouse_cityhash.cityhash import CityHash64

from alice.uniproxy.library.common_handlers import CommonRequestHandler

from alice.uniproxy.library.global_state import GlobalState
from alice.uniproxy.library.global_counter import GlobalCounter
from alice.uniproxy.library.global_counter import GlobalTimings
from alice.uniproxy.library.global_counter import UnistatTiming

from alice.uniproxy.library.async_http_client import QueuedHTTPClient, HTTPRequest, HTTPError

from mssngr.router.lib.protos.message_pb2 import TOutMessage
from alice.uniproxy.library.protos.uniproxy_pb2 import TSubwayMessage
from alice.uniproxy.library.messenger.client_locator import ClientLocator, guids_by_requests

from alice.uniproxy.library.settings import config
from alice.uniproxy.library.settings import DELIVERY_DEFAULT_FORMAT

from alice.uniproxy.library.subway.push_client import push_message
from alice.uniproxy.library.resolvers import QloudResolver, RTCResolver


DELIVERY_DEFAULT_SUBWAY_PORT = config.get('subway', {}).get('port', 7777)
DELIVERY_TVM_TOKEN = config.get('qloud_tvm_token', '')
DELIVERY_TVM_CONNECT_TIMEOUT = config.get('tvm', {}).get('checksrv_connect_timeout', 0.02)
DELIVERY_TVM_REQUEST_TIMEOUT = config.get('tvm', {}).get('checksrv_request_timeout', 0.1)
DELIVERY_TVM_HEADER_HOST = 'localhost:1'
DELIVERY_TVM_CHECKSRV_URL = 'http://localhost:1/tvm/checksrv'

QLOUD_DISCOVERY_ENABLED = config.get('delivery', {}).get('qloud_resolver', {}).get('enabled', False)
RTC_DISCOVERY_ENABLED = config.get('delivery', {}).get('rtc_resolver', {}).get('enabled', False)

if QLOUD_DISCOVERY_ENABLED and RTC_DISCOVERY_ENABLED:
    raise Exception('More than one resolver enabled')

_g_tvm_client = None


# ====================================================================================================================
class MessengerHandler(CommonRequestHandler):
    unistat_handler_name = 'delivery'

    def __init__(self, *args, **kwargs):
        global _g_tvm_client

        super(MessengerHandler, self).__init__(*args, **kwargs)
        self._logger = logging.getLogger('delivery.post')

        if QLOUD_DISCOVERY_ENABLED:
            self._resolver = QloudResolver.instance()

        if RTC_DISCOVERY_ENABLED:
            self._resolver = RTCResolver.instance()

        if _g_tvm_client is None:
            _g_tvm_client = QueuedHTTPClient(
                host="localhost",
                port=1,
                pool_size=10,
                queue_size=300,
                secure=False
            )

    # ----------------------------------------------------------------------------------------------------------------
    def prepare(self):
        self._service_ticket = self.request.headers.get('X-Ya-Service-Ticket')
        self._subway_port = self.request.headers.get('X-Subway-Port', DELIVERY_DEFAULT_SUBWAY_PORT)
        self._subway_format = self.request.headers.get('X-Subway-Format', DELIVERY_DEFAULT_FORMAT)
        self._subway_wait = self.request.headers.get('X-Subway-Wait')
        self._locator = ClientLocator()

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def _check_service_ticket(self):
        global _g_tvm_client

        try:
            request = HTTPRequest(
                "/tvm/checksrv",
                headers={
                    'Authorization':        DELIVERY_TVM_TOKEN,
                    'Host':                 DELIVERY_TVM_HEADER_HOST,
                    'X-Ya-Service-Ticket':  self._service_ticket,
                },
                request_timeout=DELIVERY_TVM_REQUEST_TIMEOUT
            )

            yield _g_tvm_client.fetch(request)

            return 200, 'ok'
        except HTTPError as e:
            try:
                error = json.loads(e.body.decode('utf-8')).get('error')
                if not error:
                    return e.code, 'no error found in response'
                return e.code, error
            except json.JSONDecodeError as ex:
                return e.code, 'failed to decode json in tvmtool response' + str(ex)
            except Exception as ex:
                return e.code, str(ex)
        except Exception as ex:
            return 0, str(ex)

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def post(self):
        self._request_id = self.request.headers.get('X-Request-Id', '0')

        if GlobalState.is_offline():
            self.set_status(503)
            self.finish('503 Service Unavailable (starting)')
            return

        s_ts = time.time()

        GlobalCounter.MSSNGR_MESSAGES_OUT_RECV_SUMM.increment()

        if self._service_ticket:
            status, message = yield self._check_service_ticket()
            if status != 200:
                GlobalCounter.MSSNGR_MESSAGES_OUT_SERVICE_TICKET_VALIDATION_ERROR_SUMM.increment()
                self._logger.error('REQ-ID %s: TVM STATUS(%d) MESSAGE(%s)', self._request_id, status, message)
                self.set_status(403)
                self.finish({
                    'component':    'tvmtool',
                    'status':       status,
                    'error':        message
                })
                GlobalTimings.store('mssngr_out_total', time.time() - s_ts)
                return
        else:
            GlobalCounter.MSSNGR_MESSAGES_OUT_NO_SERVICE_TICKET_ERROR_SUMM.increment()
            self._logger.error('REQ-ID %s NO SERVICE TICKET IN DELIVERY REQUEST', self._request_id)
            self.set_status(403)
            self.finish({
                'component':    'tvm',
                'status':       'no service ticket',
                'error':        'service ticket is required for delivery request'
            })
            GlobalTimings.store('mssngr_out_total', time.time() - s_ts)
            return

        try:
            header, data = self.request.body[:12], self.request.body[12:]
            version, checksum = struct.unpack('<IQ', header)

            if version != 2:
                GlobalCounter.MSSNGR_MESSAGES_OUT_INVALID_VERSION_SUMM.increment()
                self._logger.error('REQ-ID %s INVALID VERSION(%d)', self._request_id, version)
                self.set_status(400)
                self.finish({
                    'component':    'client',
                    'code':         400,
                    'error':        'header contains invalid version({})'.format(version),
                })
                GlobalTimings.store('mssngr_out_total', time.time() - s_ts)
                return

            real_checksum = CityHash64(data)
            if checksum != real_checksum:
                GlobalCounter.MSSNGR_MESSAGES_OUT_INVALID_VERSION_SUMM.increment()
                self._logger.error(
                    'REQ-ID %s INVALID CHECKSUM GOT(%d) CALC(%d)', self._request_id, checksum, real_checksum
                )
                self.set_status(400)
                self.finish({
                    'component':    'client',
                    'code':         400,
                    'error':        'header contains invalid checksum({})'.format(checksum),
                })
                GlobalTimings.store('mssngr_out_total', time.time() - s_ts)
                return

            fanout_message = TOutMessage()
            fanout_message.ParseFromString(data)

            self._logger.info(
                'REQ-ID %s DELIVERY-RECV %s TS:%s', self._request_id, fanout_message.PayloadId, fanout_message.Timestamp
            )

            guids = sorted(list(fanout_message.Guids))

            GlobalCounter.MSSNGR_GUIDS_COUNT_SUMM.increment(len(guids))

            with UnistatTiming('mssngr_out_resolve'):
                guids_lists = guids_by_requests(guids)
                futures = []
                for lst in guids_lists:
                    futures.append(self._locator.resolve_locations(lst))
                locations = yield futures
                for i in range(1, len(locations)):
                    locations[0].update(locations[i])
                locations = locations[0]

            self._logger.info(
                'REQ-ID %s DELIVERY-LOCATIONS %s TS:%s COUNT:%d',
                self._request_id,
                fanout_message.PayloadId,
                fanout_message.Timestamp,
                len(locations)
            )

            if len(locations) == 0:
                GlobalCounter.MSSNGR_MESSAGES_OUT_NO_LOCATION_SUMM.increment()
                self.set_status(200)
                self.finish({
                    'component':    'delivery',
                    'code':         404,
                    'error':        'no locations at all',
                })
                return

            del fanout_message.Guids[:]

            if self._subway_wait:
                yield self._deliver(locations, fanout_message)
            else:
                self.start_delivery(locations, fanout_message)

            self.set_status(200)
            self.finish({
                'code': 200,
            })

            GlobalCounter.MSSNGR_MESSAGES_OUT_SENT_SUMM.increment()
        except Exception as ex:
            GlobalCounter.MSSNGR_MESSAGES_OUT_FAIL_SUMM.increment()
            self._logger.exception(ex)
            self.set_status(500)
            self.finish({
                'component':    'delivery',
                'code':         500,
                'error':        str(ex),
            })
        finally:
            GlobalTimings.store('mssngr_out_total', time.time() - s_ts)

    # ----------------------------------------------------------------------------------------------------------------
    def start_delivery(self, locations, message: TOutMessage):
        tornado.ioloop.IOLoop.current().spawn_callback(self._deliver, locations, message)

    # ----------------------------------------------------------------------------------------------------------------
    def _get_payload_id(self, message: TOutMessage) -> str:
        payload_id = message.PayloadId
        if not payload_id:
            payload_id = message.ServerMessage.ClientMessage.Plain.PayloadId
        return payload_id

    # ----------------------------------------------------------------------------------------------------------------
    def _log_message(self, message: TOutMessage) -> str:
        ids = self._get_payload_id(message)
        if message.Timestamp != 0 or ids:
            self._logger.info('DELIVER-END %s TS:%s', ids, message.Timestamp)

    # ----------------------------------------------------------------------------------------------------------------
    def _log_fail(self, message: TOutMessage) -> str:
        self._logger.info('DELIVER-FAIL %s TS:%s', self._get_payload_id(message), message.Timestamp)

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def _deliver(self, locations, message: TOutMessage):
        if self._subway_format == 'proto':
            yield self._deliver_proto(locations, message)
            self._log_message(message)
        else:
            self._log_fail(message)
            GlobalCounter.MSSNGR_MESSAGES_OUT_FAIL_SUMM.increment()

    # ----------------------------------------------------------------------------------------------------------------
    def _get_location(self, locations):
        return locations.enumerate_locations()

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def _deliver_proto(self, locations, message: TOutMessage):
        requests = {}
        for guid, location, index in self._get_location(locations):
            alias = location

            if QLOUD_DISCOVERY_ENABLED:
                ipaddr = yield self._resolver.resolve_uniproxy(index)
                if ipaddr is not None:
                    location = ipaddr
                else:
                    continue

            if RTC_DISCOVERY_ENABLED:
                hostname = yield self._resolver.resolve_uniproxy(location)
                if hostname is not None:
                    location = hostname
                else:
                    continue

            if location not in requests:
                self._logger.debug('add location for delivery %s', location)
                subway_message = TSubwayMessage()
                subway_message.MessengerMsg.CopyFrom(message)
                requests[location] = (alias, subway_message)

            destination = requests[location][1].Destinations.add()
            destination.Guid = self.uuid_to_bytes(guid)

        yield [
            push_message(
                location,
                requests[location][1],
                port=self._subway_port,
                log=self._logger,
                alias=requests[location][0]
            )
            for location in requests
        ]

    # ----------------------------------------------------------------------------------------------------------------
    def uuid_to_bytes(self, uuid):
        return uuid_.UUID(uuid).bytes
