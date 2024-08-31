""" This module contains UniSystem class
"""
from uuid import uuid4
import copy
import datetime
import json
import os
import re
import struct
import weakref
import random

from tornado.concurrent import Future
from tornado.ioloop import IOLoop
import tornado.gen

from rtlog import begin_request, null_logger

from alice.uniproxy.library.events import Event
from alice.uniproxy.library.events import StreamControl
from alice.uniproxy.library.events import ExtraData
from alice.uniproxy.library.events import EventException
from alice.uniproxy.library.events import GoAway
from alice.uniproxy.library.events import InvalidAuth
from alice.uniproxy.library.events import Directive
from alice.uniproxy.library.events import check_stream_id

from alice.uniproxy.library.auth.blackbox import blackbox_client
from alice.uniproxy.library.auth.tvm2 import tvm_client

from alice.uniproxy.library.backends_bio import ContextStorage
from alice.uniproxy.library.backends_common.apikey import check_key
from alice.uniproxy.library.backends_ctxs.contacts import Contacts
from alice.uniproxy.library.backends_laas import get_coords
from alice.uniproxy.library.backends_tts.cache import CacheManager

from alice.uniproxy.library.extlog import SessionLogger
from alice.uniproxy.library.global_counter import GlobalCounter
from alice.uniproxy.library.global_counter import UnistatTiming
from alice.uniproxy.library.global_counter import GolovanBackend
from alice.uniproxy.library.global_counter import Unistat
from alice.uniproxy.library.global_state import GlobalState
from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.processors import create_event_processor, EventProcessor
from alice.uniproxy.library.settings import config
from alice.uniproxy.library.notificator_api import NotificatorApi
from alice.uniproxy.library.notificator_api.locator import unregister_device

from alice.uniproxy.library.utils import deepupdate
from alice.uniproxy.library.utils import security
from alice.uniproxy.library.utils.experiments import safe_experiments_vins_format
from alice.uniproxy.library.utils.experiments import weak_update_experiments

from alice.uniproxy.library.messenger.auth import MessengerAuthError, YambAuth, FanoutAuth
from alice.uniproxy.library.messenger.auth_error import MSSNGR_CLIENT_AUTH_ERROR_TEXT
from alice.uniproxy.library.messenger.msgsettings import UNIPROXY_MESSENGER_CURRENT_VERSION
from alice.uniproxy.library.messenger.msgsettings import UNIPROXY_MESSENGER_MINIMAL_VERSION

from alice.uniproxy.library.subway.pull_client import PullClient as SubwayPullClient, ClientType
from alice.uniproxy.library.subway.pull_client.singleton import subway_client

from alice.uniproxy.library.responses_storage import ResponsesStorage

UNIWS_UUID_HEADER = "X-UPRX-UUID"
UNIWS_SSID_HEADER = "X-UPRX-SSID"
UNIWS_AUTHTOKEN_HEADER = "X-UPRX-AUTH-TOKEN"
UNIWS_RTLOG_TOKEN = "X-RTLog-Token"
TURN_OFF_NOTIFICATION_FEATURE = bool(os.environ.get('TURN_OFF_NOTIFICATION_FEATURE', ''))


def _get_speechkit_version_as_int(payload):
    try:
        a, b, c = payload["speechkitVersion"].split(".")
        return int(a) * 1000000 + int(b) * 1000 + int(c)
    except:
        return -1  # absent field or unsupported version format


class UniSystem:
    """UniSystem - the main class for uni proxy functionality."""

    def __init__(self, websocket=None, ipaddr='', client_port=None, exps_check=False, session_data=None, test_ids=None):
        self._log = Logger.get('.unisystem')
        self.test_ids = []
        if test_ids:
            self.test_ids = test_ids
        self.unistat = Unistat(weakref.proxy(self))
        self.cache_manager = CacheManager()
        self.websocket = weakref.proxy(websocket) if websocket else None
        self.closed = False
        rt_log_token = self.websocket.request.headers.get(UNIWS_RTLOG_TOKEN) if self.websocket else None
        self.rt_log = begin_request(token=rt_log_token, session=True)
        self.hostname = os.uname().nodename
        self.opened_streams = {}  # stream_id -> processor
        self._max_stream_id = config["max_opened_streams"]
        self._processors = {}
        self.stream_id_counter = 0
        self.session_id = self.next_session_id()
        if self.websocket:
            sess_id = self.websocket.request.headers.get(UNIWS_SSID_HEADER)
            if sess_id:
                self.session_id = sess_id
        self.logger = SessionLogger(self.session_id, ipaddr=ipaddr, rt_log=self.rt_log, system=self)
        # write recv/sended client messages to session log
        self.session_log_client = True
        self.session_data = session_data or {}
        self.uniproxy2 = None  # MUST be None if uniproxy2 not used
        self.client_ip = ipaddr
        self.client_port = client_port
        self.user_agent = None
        self.messenger_voice_chats = []
        self.icookie = None
        self.icookie_for_uaas = None  # plain (decrypted) ICookie - from client or UUID-generated
        self.y_session_id = None
        self.y_yamb_session_id = None
        self.log_spotter_vins_message_id = None

        self.rt_log.log_request_context(
            uniproxy_session_id=self.session_id,
            client_ip=self.client_ip)

        self.init_futures = {}
        self.saved_error = None

        self.exps_check = exps_check

        self.origin = ""
        self.uid = None
        self.puid = None
        self.yuid = None
        self.guid = None
        self.yandex_login = None
        self.oauth_token = None
        self.event_patcher = None
        self.experiments = []
        self.uaas_flags = {}
        self.uaas_test_ids = []
        self.uaas_exp_boxes = None
        self.uaas_asr_flags = None
        self.uaas_bio_flags = None
        self.suspended_messages = None
        self.staff_login = None
        self.timestamp_started = datetime.datetime.now()
        self.close_required = False
        self.suspend_calls = 0
        self.suspend_future = None
        self.wifinets = []

        self.x_yamb_token = None
        self.x_yamb_token_type = None
        self.x_yamb_cookie = None
        self.csrf_token = None
        self.fanout_auth = False
        self.mssngr_initialized = False
        self.mssngr_version = None
        self.mssngr_user_is_fake = False
        self.mssngr_user_is_anon = False
        self.mssngr_counter = None

        self.srcrwr = None  # type: Srcrwr
        self.graph_overrides = None  # type: GraphOverrides

        self.synchronize_state_timer = None
        self.synchronize_state_finished_timer = None
        self.synchronize_state_message_id = ''

        self.total_processors_count = 0

        self.mssngr_auth_error = None

        self.contacts = Contacts()
        self.asr_help_ctxs = None

        self.save_to_mds = True
        self.device_id = None
        self.platform = None
        self.x_yandex_appinfo = None
        self._app_type = None
        self._app_id = None
        self._do_not_use_user_logs = None
        self.device_model = None
        self.device_manufacturer = None

        self.goaway_sent = False
        self._yabio_storage = {}
        self.subway_client_type = None

        self.bb_uid4oauth_error = False
        # SK-3572 Send GoAway after 9 minutes
        self.goaway_timeout = IOLoop.current().call_later(config.get("goaway_timeout", 540), self.send_goaway)

        self.ab_asr_topic = None
        self.user_ticket_future = None

        self.use_balancing_hint = True
        self.use_spotter_glue = False
        self.use_laas = config.get('laas', {}).get('enable', True)
        self.use_datasync = config.get('data_sync', {}).get('enable', True)
        self.use_personal_cards = config.get('personal_cards', {}).get('enable', True)

        self.responses_storage = ResponsesStorage()

        self.acks = {}

    @property
    def is_robot(self):
        uuid = self.uuid()
        if not uuid:
            return False
        u = Unistat(self)
        u.check_uuid(uuid)
        return u.is_robot

    @property
    def is_quasar(self):
        return self.session_data.get("key", "") == "51ae06cc-5c8f-48dc-93ae-7214517679e6"

    @property
    def subway_uid(self):
        if self.subway_client_type == ClientType.DEVICE:
            return self.device_id
        elif self.subway_client_type == ClientType.GUID:
            return self.guid
        return None

    def is_feature_supported(self, feature):
        return feature in self.session_data.get('supported_features', [])

    def is_notification_supported(self):
        if TURN_OFF_NOTIFICATION_FEATURE:
            return False
        return self.is_feature_supported('notifications')

    def send_goaway(self, message_id=None):
        if not self.goaway_sent:
            self.write_directive(GoAway(message_id))
            self.goaway_sent = True

    def process_headers(self):
        headers = {}

        uuid = self.websocket.request.headers.get(UNIWS_UUID_HEADER)
        auth_token = self.websocket.request.headers.get(UNIWS_AUTHTOKEN_HEADER)
        token = self.websocket.request.headers.get("Authorization")
        if uuid:
            self.set_uuid(uuid)
            headers["uuid"] = uuid

        if auth_token:
            self.set_auth_token(auth_token)
            headers["auth_token"] = auth_token

        if token:
            self.set_oauth_token(token)
            headers["oauth_token"] = token

        self.logger.log_directive(
            {
                "type": "Headers",
                "headers": headers
            }
        )

    @tornado.gen.coroutine
    def wait_initialization(self, message_id=None):
        self.suspend_message_processing(True, 'wait_initialization')
        try:
            yield list(self.init_futures.values())
        except Exception as exc:
            self.saved_error = str(exc)
            self.write_directive(EventException(self.saved_error, event_id=message_id))
            self.increment_stats('event_exceptions')
        self.suspend_message_processing(False, 'wait_initialization')

    def set_uuid(self, uuid):
        if not uuid and "uuid" in self.session_data:
            # already got uuid from headers
            return

        fut = self.init_futures["uuid"] = Future()

        uuid = uuid.lower() if uuid else ""

        self.rt_log.log_request_context(uuid=uuid)
        self.logger.set_uuid(uuid)
        if re.match(r"^[0-9a-f]{8}-?([0-9a-f]{4}-?){3}[0-9a-f]{12}$", uuid):
            self.session_data["uuid"] = uuid
            fut.set_result("ok")
            self.unistat.check_uuid(uuid)
        else:
            fut.set_exception(Exception("Invalid uuid"))

        if uuid[:16] == 'f' * 16 or config.get("disable_session_log"):
            self.logger.disable()

    def set_csrf_token(self, token):
        self.csrf_token = token

    def set_messenger_data(self, data):
        self.mssngr_version = data.get('version', 0)
        self.fanout_auth = data.get('fanout_auth', False)
        self.mssngr_min_version = data.get('debug_params', {}).get(
            'overriden_minimal_version',
            UNIPROXY_MESSENGER_MINIMAL_VERSION
        )
        self.mssngr_cur_version = data.get('debug_params', {}).get(
            'overriden_current_version',
            UNIPROXY_MESSENGER_CURRENT_VERSION
        )

    def set_synchronize_state_id(self, message_id):
        self.synchronize_state_message_id = message_id

    def set_auth_token(self, key, service_name=None):
        if (not key) and ("apiKey" in self.session_data):
            return

        fut = self.init_futures["apiKey"] = Future()

        def on_ok(key):
            self.session_data["apiKey"] = self.session_data["key"] = key
            fut.set_result("ok")

        def on_fail():
            fut.set_exception(Exception("Invalid auth_token"))

        check_key(
            key,
            client_ip=self.client_ip,
            on_ok=on_ok,
            on_fail=on_fail,
            rt_log=self.rt_log,
            service_name=service_name,
        )

    @tornado.gen.coroutine
    def set_oauth_token(self, oauth_token, yandexuid=None, event_id=None):
        self.yuid = yandexuid
        if oauth_token:
            yield self._get_uid(oauth_token, event_id)
        else:
            self.uid = yandexuid
            if self.uid:
                self.send_uniproxy2_update_user_session(uid=self.uid)

        if self.use_laas:
            self._start_laas_request()

    def get_oauth_token(self):
        return self.oauth_token

    @tornado.gen.coroutine
    def get_bb_user_ticket(self):
        if (self.oauth_token is None) or (self.client_ip is None):
            return None

        try:
            if not self.user_ticket_future:
                @tornado.gen.coroutine
                def _wrapper():
                    res = yield blackbox_client().ticket4oauth(
                        self.oauth_token,
                        self.client_ip,
                        self.client_port,
                        rt_log=self.rt_log,
                    )
                    return res

                self.user_ticket_future = _wrapper()

            ticket = yield self.user_ticket_future
            return ticket
        except Exception as e:
            self.WARN("can't get user ticket", e)
        finally:
            self.user_ticket_future = None

        return None

    def session_ready(self):
        return self.session_data.get("key") and self.session_data.get("uuid")

    def update_session(self, data):
        deepupdate(self.session_data, data, copy=False)
        self.unistat.is_quasar = self.is_quasar

    def set_x_yandex_appinfo(self, val):
        self.x_yandex_appinfo = val
        self.update_session({
            "request": {
                "additional_options": {
                    "app_info": self.x_yandex_appinfo
                }
            }
        })

    def next_session_id(self):
        return str(uuid4())

    def next_message_id(self):
        return str(uuid4())

    def next_stream_id(self):
        """ Returns next streamId
            server streams have even (%2==0) identificators
        """
        self.stream_id_counter += 2
        return self.stream_id_counter

    def write_message(self, msg, log_message=True):
        if self.session_log_client and log_message:
            self.logger.log_directive(msg)
        try:
            self.websocket.write_message(msg)
        except Exception as exc:
            self.WARN("Can't write message:", exc)

    def write_directive(self, directive, processor=None, log_message=True):
        self.write_message(directive.create_message(weakref.proxy(self)), log_message)
        if directive.stream_id is not None:
            if processor is not None:
                self.opened_streams[directive.stream_id] = processor
            self.logger.start_stream(
                directive.message_id,
                directive.stream_id,
                directive.payload.get("format", "pcm16")
            )
        return directive

    def write_data(self, stream_id, data):
        prepend = struct.pack(">I", stream_id)
        try:
            self.websocket.write_message(prepend + data, binary=True)
        except Exception as exc:
            self.WARN("Can't write data stream:", exc)
        # self.logger.log_data(stream_id, data)

    def write_streamcontrol(self, streamcontrol):
        self.write_message(streamcontrol.create_message(weakref.proxy(self)))

    def close_stream(self, stream_id):
        self.write_streamcontrol(StreamControl.okClose(weakref.proxy(self), stream_id))
        self.logger.close_stream(stream_id)
        if stream_id in self.opened_streams:
            self.DLOG("close streamId={}".format(stream_id))
            self.opened_streams.pop(stream_id)

    def on_message(self, message):
        try:
            if self.suspended_messages is not None:
                self.suspended_messages.append(message)
                return

            if isinstance(message, bytes):
                self.DLOG("Data message")
                self.process_data_message(message)
            else:
                self.process_json_message(message)
        except EventException as exp:
            self.write_directive(exp)
            self.rt_log.error('exception_sent_to_client', message=exp.message)
            GlobalCounter.EVENT_EXCEPTIONS_SUMM.increment()
            self.increment_stats('event_exceptions')
        except Exception as exp:
            self.ERR("Websocket error: %s" % (exp,))
            self.EXC(exp)

    def amend_event_payload(self, payload):
        request = payload.get('request')
        if not request:
            return
        if 'experiments' in request:
            # VOICESERV-1949 experiments MUST BE stored as dict
            request['experiments'] = safe_experiments_vins_format(request['experiments'], self.WARN)
        if 'megamind_cookies' in request and request['megamind_cookies']:
            try:
                megamind_cookies = json.loads(request['megamind_cookies'])
                if 'uaas_cookie' in megamind_cookies:
                    payload['cookie'] = megamind_cookies['uaas_cookie']
                elif 'uaas_tests' in megamind_cookies:
                    payload['uaas_tests'] = megamind_cookies['uaas_tests']
            except Exception as ex:
                self.WARN(ex)

    def process_experiments(self, event, rt_log=null_logger()):
        self.amend_event_payload(event.payload)
        if self.uaas_flags:
            # VOICESERV-1949: priority for '/request/experiments' only: 1. client  2. uaas  3. local experiments
            experiments = event.payload.setdefault("request", {}).setdefault("experiments", {})
            # VOICESERV-3145. See usage in alice.uniproxy.library.utils.experiments.conducting_experiment
            event.payload["request"]["experiments_without_uaas_flags"] = copy.deepcopy(experiments)
            weak_update_experiments(experiments, self.uaas_flags)
        if self.event_patcher:
            self.event_patcher.patch(event, self.session_data, staff_login=self.staff_login, rt_log=self.rt_log)

        if event.stream_id and self._max_stream_id < len(self.opened_streams):
            raise GoAway(event.message_id)

    def process_event(self, event, rt_log=null_logger(), on_create_processor=None):
        if GlobalState.is_offline() and "enable_goaway" in self.session_data:
            self.send_goaway(event.message_id)

        if self.saved_error:
            raise EventException(self.saved_error, event.message_id)

        # VOICESERV-4124
        # stream_ids may be global for session.
        # Therefore the only way to allow more chanels is to increase total limit.
        if (event.namespace, event.name) == ("Log", "Spotter"):
            if "max_opened_streams_for_log_spotter" in config:
                self._max_stream_id = config["max_opened_streams_for_log_spotter"]

        if event.namespace != 'Messenger':
            self.process_experiments(event, rt_log)

        # VOICESERV-2397
        if '' in event.payload.get('request', {}).get('experiments', {}):
            del event.payload['request']['experiments']['']

        self.DLOG("creating processor", rt_log=rt_log)
        processor = create_event_processor(weakref.proxy(self), event, rt_log)
        if processor.proc_id in self._processors:
            self.ERR(f"Duplicated processor ID {processor.proc_id}")
            raise EventException("duplicated message ID", event.message_id)

        self._processors[processor.proc_id] = processor
        self.total_processors_count += 1
        if on_create_processor:
            on_create_processor(processor)
        self.DLOG("processor created" + (' with streamId={}'.format(repr(event.stream_id)) if event.stream_id else ''),
                  rt_log=rt_log)

        if event.stream_id:
            self.logger.start_stream(
                event.message_id,
                event.stream_id,
                event.payload.get("format", self.session_data.get("format", "pcm16")),
                self.save_to_mds,
            )
            self.opened_streams[event.stream_id] = processor
        self.DLOG("Creating processor %s" % (processor,), rt_log=rt_log)

        processor.process_event(event)
        return processor

    def on_close_event_processor(self, processor):
        try:
            self._processors.pop(processor.proc_id, None)  # remove link to finished processors
            # we can not remove items from dict on-the-fly, so use list for collect keys
            for k in list(k for k, v in self.opened_streams.items() if v is processor):
                del self.opened_streams[k]
            self.send_uniproxy2_event_processor_finished(processor)
        except ValueError:
            pass  # ignore repeat finalization

    def process_json_message(self, message):
        event, streamcontrol, extra = self.parse_json_message(message)
        if event is not None and event.event_type() not in ["system.synchronizestate", "system.setstate"] and not self.session_ready():
            self.ERR(event.event_type())
            self.ERR(security.hide_dict_tokens(self.session_data))
            raise EventException("No SynchronizeState was sent", event.message_id)

        if streamcontrol:
            if not self.session_ready():
                raise EventException("No SynchronizeState was sent", streamcontrol.message_id)
            try:
                if streamcontrol.stream_id in self.opened_streams:
                    self.DLOG("close [2] streamId={}".format(streamcontrol.stream_id))
                    action = self.opened_streams[streamcontrol.stream_id].process_streamcontrol(streamcontrol)
                    if action == EventProcessor.StreamControlAction.Close:
                        if streamcontrol.stream_id in self.opened_streams:
                            self.opened_streams.pop(streamcontrol.stream_id)
                        self.logger.close_stream(streamcontrol.stream_id)
                    elif action == EventProcessor.StreamControlAction.Flush:
                        self.logger.flush_stream(streamcontrol.stream_id)
            except KeyError as exc:
                raise EventException("No such streamId: %s" % (streamcontrol.stream_id,), streamcontrol.message_id, exc)

        if event:
            self.DLOG('processing event...')

            if event.rtlog_token:
                rt_log = begin_request(token=event.rtlog_token, session=False)
            else:
                rt_log = self.rt_log.create_request_logger(event.event_type())
            try:
                self.process_event(event, rt_log)
            except EventException as e:
                rt_log.exception('will_send_exception_to_client')
                if e.initial_exception:
                    rt_log.exception_info(e.initial_exception, 'initial_exception')
                raise
            except Exception:
                rt_log.exception('unexpected_exception')
                raise

        if extra:
            proc = self._processors.get(extra.proc_id)
            if proc is not None:
                try:
                    proc.process_extra(extra)
                    GlobalCounter.EXTDATA_PROCESSED_SUMM.increment()
                except Exception:
                    GlobalCounter.EXTDATA_DROPPED_SUMM.increment()
                    self.EXC(f"{extra} dropped due to exception")
            else:
                GlobalCounter.EXTDATA_DROPPED_SUMM.increment()
                self.ERR(f"{extra} dropped due to absence of target processor")

    def process_data_message(self, data):
        if len(data) < 4:
            raise EventException("Data package is too small, probably [stream_id] is missed.")

        stream_id = struct.unpack(">I", data[:4])[0]
        check_stream_id(stream_id)
        if stream_id in self.opened_streams:
            try:
                true_data = data[4:]
                self.logger.log_data(stream_id, true_data)
                self.opened_streams[stream_id].add_data(true_data)
            except Exception as exc:
                raise EventException("Could not add data to stream.", None, exc)
        else:
            self.DLOG("Bad streamId={}".format(repr(stream_id)))

    def parse_json_message(self, message):
        obj = None
        try:
            obj = json.loads(message)
            if self.session_log_client:
                # hack for disable SynchornizeState logging
                uniproxy2_write_session_log = False
                if not self.synchronize_state_message_id:
                    try:
                        uniproxy2_write_session_log = obj['event']['payload']['uniproxy2']['session_log']
                    except:
                        pass
                if not uniproxy2_write_session_log:
                    self.logger.log_rawmessage(obj)
        except Exception as exc:
            self.logger.log_rawmessage(message)
            raise EventException("Bad json message formatting.", None, exc)

        return (Event.try_parse(obj), StreamControl.try_parse(obj), ExtraData.try_parse(obj))

    def set_event_patcher(self, ep):
        self.event_patcher = ep

    def log_experiment(self, descr):
        self.experiments.append(descr)
        if not self.exps_check:
            self.logger.log_experiment(descr)

    def log_exp_boxes(self, exp_boxes):
        self.logger.log_exp_boxes(exp_boxes)

    def on_synchronize_state(self):
        self.synchronize_state_timer = UnistatTiming('synchronize_state_wait').start()

    def suspend_message_processing(self, susp, reason=''):
        if susp:
            self.suspend_calls += 1
            if self.suspend_future is None:
                self.suspend_future = Future()
            self.DLOG('suspend message processing for <{}>'.format(reason))
            if self.suspended_messages is None:
                self.suspended_messages = []
        else:
            self.suspend_calls -= 1
            self.DLOG('step to resume message processing (finish <{}>)'.format(reason))

            if self.suspend_calls != 0:
                return

            if self.synchronize_state_timer:
                self.synchronize_state_timer.stop()
                self.synchronize_state_finished_timer = self.synchronize_state_timer
                self.synchronize_state_timer = None

            self.send_synchronize_state_response()
            self.suspend_future.set_result(True)
            self.suspend_future = None

            if self.suspended_messages is not None:
                self.DLOG('resume message processing (finish {})'.format(reason))
                messages = self.suspended_messages
                self.suspended_messages = None
                for msg in messages:
                    self.on_message(msg)

    def _send_backend_versions(self):
        self.DLOG('_send_backend_versions')
        self.write_directive(Directive(
            "Messenger",
            "BackendVersions",
            {
                "minimal": self.mssngr_min_version,
                "current": self.mssngr_cur_version,
            },
            self.synchronize_state_message_id
        ))

    def _send_messenger_synchronize_state_response(self):
        self.DLOG('_send_messenger_synchronize_state_response')
        payload = {
            "guid": self.guid,
            "versions": {
                "minimal": self.mssngr_min_version,
                "current": self.mssngr_cur_version,
            }
        }
        if (self.guid is None) and (self.mssngr_auth_error is not None):
            payload["error"] = self.mssngr_auth_error
        self.write_directive(Directive(
            "Messenger",
            "SynchronizeStateResponse",
            payload,
            self.synchronize_state_message_id
        ))

    def _send_system_synchronize_state_response(self):
        self.DLOG('_send_system_synchronize_state_response')
        payload = {"SessionId": self.session_id}
        if self.guid is not None:
            payload["guid"] = self.guid
        self.write_directive(Directive(
            "System",
            "SynchronizeStateResponse",
            payload,
            self.synchronize_state_message_id
        ))

    def send_synchronize_state_response(self):
        # for messenger
        if self.mssngr_initialized:
            self.DLOG('send_synchronize_state_response')
            if self.mssngr_version is None:
                return
            elif self.mssngr_version == 0:
                return
            elif self.mssngr_version <= 2:
                self._send_backend_versions()
            elif self.mssngr_version >= 3:
                self._send_messenger_synchronize_state_response()

        # for speechkitVersion >= 4.6.0
        if _get_speechkit_version_as_int(self.session_data) >= 4006000 \
           and self.mssngr_version is None:
            self._send_system_synchronize_state_response()

    def uuid(self):
        return self.session_data.get('uuid')

    def send_uniproxy2_update_user_session(self, uid=None, do_not_use_user_logs=None):
        if not self.has_uniproxy2():
            return

        payload = {}
        if uid:
            payload['uid'] = uid
        if do_not_use_user_logs is not None:
            payload['do_not_use_user_logs'] = do_not_use_user_logs
        if len(payload) == 0:
            return

        self.write_directive(Directive(
            'Uniproxy2',
            'UpdateUserSession',
            payload,
            event_id=self.synchronize_state_message_id,
        ))

    def send_uniproxy2_event_processor_finished(self, processor):
        if not self.has_uniproxy2() or not processor.event or processor.event.internal:
            return

        event = processor.event
        payload = {
            'event_full_name': event.full_name(),
        }
        self.write_directive(Directive(
            'Uniproxy2',
            'EventProcessorFinished',
            payload,
            event_id=event.message_id,
        ))

    @tornado.gen.coroutine
    def on_close_subway(self, subway: SubwayPullClient):
        self.DLOG('on_close_subway')
        if self.subway_uid is not None:
            if self.subway_client_type == ClientType.DEVICE:
                # fork coroutine
                host = self.srcrwr['NOTIFICATOR'] if self.srcrwr else None
                unregister_device(host, self.puid, self.device_id)
            yield subway.remove_client(weakref.proxy(self))

    def on_subway_message(self, data: dict):
        if self.subway_client_type == ClientType.GUID:
            namespace = 'Messenger'
            name = 'Fanout'
        else:
            namespace = 'System'
            name = 'Push'

        self.process_event(Event({
            'header': {
                'namespace': namespace,
                'name': name,
                'messageId': self.next_message_id(),
            },
            'payload': {
                'data': data,
                'transfer_id': str(uuid4()),
            }
        }, internal=True))

    def sync_state_complete(self):
        if "enable_goaway" not in self.session_data:
            self.remove_goaway_timeout()
        elif "goaway_timeout" in self.session_data:
            try:
                ga_timeout = int(self.session_data["goaway_timeout"])
                self.remove_goaway_timeout()
                self.goaway_timeout = IOLoop.current().call_later(ga_timeout, self.send_goaway)
            except Exception:
                pass
        self.DLOG('sync_state_complete')

    @tornado.gen.coroutine
    def send_notification_state(self, on_connect=False):
        # send notification state after device connected
        if not self.is_notification_supported():
            self.INFO('notification is not supported')
            return
        if self.puid is None:
            self.INFO('yandex uid is none')
            return

        self.INFO('try send new notification state')

        # fork coroutine
        yield tornado.gen.sleep(random.randint(0, 10))
        NotificatorApi(
            self.get_oauth_token(),
            self.client_ip,
            rt_log=self.rt_log,
            url=self.srcrwr['NOTIFICATOR'] if self.srcrwr else None,
            app_id=self.app_id,
            metrics_backend=GolovanBackend.instance(),
        ).on_connect(self.puid, self.device_id, self.device_model)

    def close(self):
        self.closed = True
        subway = subway_client()
        if subway:
            IOLoop.current().spawn_callback(self.on_close_subway, subway)
        else:
            self.ERR('close: subway client is null')

        if self.mssngr_counter is not None:
            self.mssngr_counter.decrement()
        self.mssngr_counter = False  # needed to avoid counting within closed UniSystem

        processors = [p for p in self._processors.values()]
        for processor in processors:
            if processor.reply_timedout():
                GlobalCounter.EVENT_REPLY_TIMEDOUT_SUMM.increment()
            processor.close()
        self.opened_streams = {}
        self._processors = {}

        self.remove_goaway_timeout()
        # pass rt_log ownership to SessionLogger
        self.logger.close()

    def remove_goaway_timeout(self):
        if self.goaway_timeout is not None:
            IOLoop.current().remove_timeout(self.goaway_timeout)
            self.goaway_timeout = None

    def has_processors(self):
        return len(self._processors) != 0

    def DLOG(self, *args, rt_log=None):
        self._log.debug(self.session_id, *args, rt_log=rt_log or self.rt_log)

    def INFO(self, *args, rt_log=None):
        self._log.info(self.session_id, *args, rt_log=rt_log or self.rt_log)

    def WARN(self, *args, rt_log=None):
        self._log.warning(self.session_id, *args, rt_log=rt_log or self.rt_log)

    def ERR(self, *args, rt_log=None):
        self._log.error(self.session_id, *args, rt_log=rt_log or self.rt_log)

    def EXC(self, exception, rt_log=None):
        self._log.exception(exception, rt_log=rt_log or self.rt_log)

    # ----------------------------------------------------------------------------------------------------------------
    def set_wifi_nets(self, networks):
        self.wifinets = networks

    # ----------------------------------------------------------------------------------------------------------------
    def _start_laas_request(self):
        if self.exps_check:
            return

        fut = self.init_futures["laas"] = Future()

        def on_ok(response):
            self.update_session({
                "request": {
                    "laas_region": response
                }
            })
            fut.set_result("ok")

        def on_err(response):
            fut.set_result("failed")

        get_coords(
            self.client_ip,
            on_ok=on_ok,
            on_err=on_err,
            rt_log=self.rt_log,
            uuid=self.uuid(),
            puid=self.puid,
            wifinets=self.wifinets,
            yandexuid=self.yuid,
            is_quasar=self.is_quasar
        )

    # ----------------------------------------------------------------------------------------------------------------
    def _preprocess_oauth_token(self, token):
        if not token:
            self.DLOG('no token')
            return False, None, None

        token_lower = token.lower()

        if token_lower.startswith('oauthteam'):
            token = token[9:].strip()
            token_type = 'OauthTeam'
            do_passport_request = False
        elif token_lower.startswith('yambauth'):
            token = token[8:].strip()
            token_type = 'yambauth'
            do_passport_request = False
        elif token_lower.startswith('oauth'):
            token = token[5:].strip()
            token_type = 'Oauth'
            do_passport_request = True
        else:
            token_type = 'Oauth'
            do_passport_request = True

        return do_passport_request, token_type, token

    # ----------------------------------------------------------------------------------------------------------------
    def get_auth_client_error(self, token, cookie):
        return MSSNGR_CLIENT_AUTH_ERROR_TEXT.get(
            (self.fanout_auth, not token, not cookie, not self.csrf_token),
            'undefined'
        )

    # ----------------------------------------------------------------------------------------------------------------
    def update_messenger_guid(self, token):
        _, token_type, token = self._preprocess_oauth_token(token)

        cookie = None

        if not token or not token_type:
            self.WARN('no token was found for this session')

            token = self.x_yamb_token
            token_type = self.x_yamb_token_type
            cookie = self.x_yamb_cookie

            if not token and not cookie:
                self.mssngr_auth_error = self.get_auth_client_error(token, cookie)
                self.WARN(self.mssngr_auth_error.get('text') if self.mssngr_auth_error else 'yamb/fanout auth no data')
                return

        self.suspend_message_processing(True, 'messenger_guid')
        IOLoop.current().spawn_callback(
            self._get_messenger_guid,
            token,
            token_type,
            self.y_session_id,
            self.y_yamb_session_id,
            self.icookie
        )

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def set_messenger_guid(self, guid, ticket, anonymous=False):
        try:
            self.mssngr_user_is_anon = False
            self.mssngr_user_is_fake = True
            self.suspend_message_processing(True, 'messenger_guid')
            self.guid = guid
            if not anonymous:
                yield tvm_client().check_service_ticket(ticket)
                self.subway_client_type = ClientType.GUID
                yield subway_client().add_client(weakref.proxy(self))
                self._count_messenger_client(GlobalCounter.MSSNGR_FAKE_CLIENTS_AMMX)
            else:
                self._count_messenger_client(GlobalCounter.MSSNGR_ANON_CLIENTS_AMMX)
                self.mssngr_user_is_anon = True
                self.mssngr_user_is_fake = False
                self.logger.disable()
            self.mssngr_initialized = True
        except Exception as ex:
            self.ERR(ex)
            self.guid = None
        finally:
            self.suspend_message_processing(False, 'messenger_guid')

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def _get_messenger_guid(self, token, token_type, y_session_id, y_yamb_session_id, icookie):
        try:
            self.DLOG('creating auth client...')
            if self.fanout_auth:
                auth_client = FanoutAuth()
            else:
                auth_client = YambAuth()

            self.DLOG('getting guid for token_type={}'.format(token_type))
            had_guid = self.guid is not None
            self.guid = yield auth_client.auth_user(
                token=token,
                token_type=token_type,
                y_cookie=y_session_id,
                y_yamb_cookie=y_yamb_session_id,
                icookie=icookie,
                ip=self.client_ip,
                port=self.client_port,
                session_id=self.session_id,
                origin=self.origin
            )

            self.subway_client_type = ClientType.GUID
            if self.guid is not None:
                yield subway_client().add_client(weakref.proxy(self))
                if not had_guid:
                    self._count_messenger_client(GlobalCounter.MSSNGR_CLIENTS_AMMX)
        except MessengerAuthError as ex:
            self.mssngr_auth_error = ex.message
        except ReferenceError as ex:
            Logger.get().warning('race(UniSystem destruction while init. messenger client ?!): {}'.format(ex))
        except Exception as ex:
            self.ERR(ex)
        else:
            return self.guid
        finally:
            self.mssngr_initialized = True
            self.suspend_message_processing(False, 'messenger_guid')
        return None

    def _count_messenger_client(self, counter):
        # client counter must not be incremented more than once
        if self.mssngr_counter is None:
            self.mssngr_counter = counter
            counter.increment()

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def set_client(self):
        if not self.is_quasar or not self.is_notification_supported():
            return

        if not self.puid:
            self.ERR('puid is not set for quasar')
            return

        try:
            self.subway_client_type = ClientType.DEVICE
            yield tornado.gen.sleep(random.randint(0, 30))
            yield subway_client().add_client(weakref.proxy(self))

            # send notification state after connect
            self.send_notification_state(on_connect=True)
        except Exception as ex:
            self.ERR(ex)

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def _get_uid(self, oauth_token, event_id):
        do_passport_request, token_type, token = self._preprocess_oauth_token(oauth_token)
        if token_type == 'Oauth':
            self.oauth_token = token
            #  VOICESERV-1287 proxy oauth_token to vins request
            self.session_data.setdefault("request", {}).setdefault(
                "additional_options", {}).setdefault("oauth_token", token)

        if do_passport_request:
            fut = self.init_futures["oauth"] = Future()
            try:
                self.puid, self.staff_login = yield blackbox_client().uid4oauth(
                    token,
                    self.client_ip,
                    self.client_port,
                    self.rt_log
                )
                if event_id:
                    self.logger.log_directive(
                        {
                            'ForEvent': event_id,
                            'GotPuid': self.puid,
                            'Type': 'from BB'
                        },
                        rt_log=self.rt_log,
                    )
                else:
                    self.logger.log_directive(
                        {
                            'GotPuid': self.puid,
                            'Type': 'from BB'
                        },
                        rt_log=self.rt_log,
                    )

                self.uid = self.puid if self.puid else self.yuid
            except Exception as exc:
                # bad oauth_token shouldn't stop processing
                self.bb_uid4oauth_error = True
                if self.session_data.get("accept_invalid_auth"):
                    self.write_directive(InvalidAuth(str(exc), self.synchronize_state_message_id))
                self.WARN("Bad blackbox response:", exc)

            fut.set_result("ok")
        elif self.yuid is not None:
            self.uid = self.yuid
            self.send_uniproxy2_update_user_session(uid=self.uid)

    @property
    def app_type(self):
        return self._app_type

    @app_type.setter
    def app_type(self, value):
        self._app_type = value
        self.logger.app_type = value

    @property
    def app_id(self):
        return self._app_id

    @app_id.setter
    def app_id(self, value):
        self._app_id = value
        self.logger.app_id = value

    @property
    def do_not_use_user_logs(self):
        return self._do_not_use_user_logs

    @do_not_use_user_logs.setter
    def do_not_use_user_logs(self, value):
        self._do_not_use_user_logs = value
        self.send_uniproxy2_update_user_session(do_not_use_user_logs=self._do_not_use_user_logs)
        self.logger.do_not_use_user_logs = value
        self.logger.uid = self.uid

    def set_uniproxy2(self, uniproxy2):
        if uniproxy2 is None:
            return

        try:
            if uniproxy2.get('session_log', False):
                # disable SESSION_LOG for client i/o messages (it will do uniproxy2)
                self.session_log_client = False
            self.uniproxy2 = uniproxy2
        except Exception as exc:
            self.WARN("Bad payload.uniproxy2: {}".format(exc))

    def has_uniproxy2(self):
        return self.uniproxy2 is not None

    def increment_stats(self, event_type, code=None):
        if code:
            if self.app_type:
                GlobalCounter.increment_error_code(event_type, code, self.app_type)

            if self.device_model:
                GlobalCounter.increment_error_code(event_type, code, self.device_model)
        else:
            if self.app_type:
                GlobalCounter.increment_counter(event_type, self.app_type)

            if self.device_model:
                GlobalCounter.increment_counter(event_type, self.device_model)

    def get_yabio_storage(self, group_id):
        ret = self._yabio_storage.get(group_id)
        if ret is None:
            ret = self._yabio_storage[group_id] = ContextStorage(
                self.session_id,
                rt_log=self.rt_log,
                group_id=group_id,
                dev_model=self.device_model,
                dev_manuf=self.device_manufacturer,
            )
        return ret

    def try_update_activity_check_period_seconds(self, new_value, variation=None):
        if self.websocket and isinstance(new_value, int) and (0 < new_value):
            if not (isinstance(variation, int) and 0 < variation):
                variation = min(60, new_value // 2)
            self.websocket.activity_check_period_seconds = new_value + random.randint(-variation, variation)
            return True
        return False
