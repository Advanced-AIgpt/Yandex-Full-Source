import time

import tornado.gen
import tornado.ioloop

from mssngr.router.lib.protos.message_pb2 import TInMessage, TResponse
from mssngr.router.lib.protos.message_pb2 import THistoryRequest, THistoryResponse
from mssngr.router.lib.protos.message_pb2 import TEditHistoryRequest, TEditHistoryResponse
from mssngr.router.lib.protos.message_pb2 import TSubscriptionRequest, TSubscriptionResponse
from mssngr.router.lib.protos.message_pb2 import TWhoamiRequest, TWhoamiResponse
from mssngr.router.lib.protos.message_pb2 import TMessageInfoRequest, TMessageInfoResponse

from alice.uniproxy.library.messenger import mssngr_client_instance
from alice.uniproxy.library.messenger.exception import AuthenticationError
from alice.uniproxy.library.messenger.exception import MessengerError

from alice.uniproxy.library.async_http_client import HTTPError

from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.global_counter import UnistatTiming, GlobalCounter

from alice.uniproxy.library.extlog import AccessLogger
from alice.uniproxy.library.backends_asr import YaldiStream

from alice.uniproxy.library.events import Event
from alice.uniproxy.library.events import Directive

from alice.uniproxy.library.processors.asr import Recognize
from . import EventProcessor, register_event_processor


UNIPROXY_MSSNGR_AUTH_WAIT_RETRIES = 10
UNIPROXY_MSSNGR_AUTH_WAIT_TIME = 0.03


# ====================================================================================================================
class MessengerProcessorBase(EventProcessor):
    def __init__(self, *args, **kwargs):
        super(MessengerProcessorBase, self).__init__(*args, **kwargs)

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def wait_for_auth(self):
        if self.system.guid:
            self.event.payload['Guid'] = self.system.guid
            return

        for i in range(UNIPROXY_MSSNGR_AUTH_WAIT_RETRIES):
            yield tornado.gen.sleep(UNIPROXY_MSSNGR_AUTH_WAIT_TIME)

            if self.system.guid:
                self.event.payload['Guid'] = self.system.guid
                return

        raise AuthenticationError()

    # ----------------------------------------------------------------------------------------------------------------
    def process_event(self, event):
        super().process_event(event)

        if 'CommonRequestFields' not in event.payload:
            event.payload['CommonRequestFields'] = {}

        event.payload['UserIp'] = self.system.client_ip
        event.payload['CommonRequestFields']['UserIp'] = self.system.client_ip

        if self.system.user_agent:
            event.payload['UserAgent'] = self.system.user_agent

        if self.system.icookie:
            log_data = event.payload.setdefault("ClientMessage", {}).setdefault("LogData", {})
            log_data["ICookie"] = self.system.icookie

    # ----------------------------------------------------------------------------------------------------------------
    def make_directive(self, event, payload, exception=False):
        if exception:
            return Directive('System', 'EventException', payload, event.message_id)
        else:
            return Directive('Messenger', self._response_name, payload, event.message_id)

    # ----------------------------------------------------------------------------------------------------------------
    def make_auth_error(self, err):
        payload = {
            'error': {
                'type': 'Error',
                'message': 'unauthorized messenger user'
            }
        }

        if self.system.mssngr_auth_error:
            payload.update({
                'details': self.system.mssngr_auth_error,
            })

        return self.make_directive(self.event, payload, exception=True)

    # ----------------------------------------------------------------------------------------------------------------
    def make_messenger_error(self, err):
        if err.raw:
            payload = err.response
        else:
            payload = {
                'error': {
                    'type': 'Error',
                    'message': str(err.code),
                },
                'details': {
                    'scope': 'fanout',
                    'code': str(err.code),
                    'text': 'fanout error'
                },
                'Response': err.response,
            }

        return self.make_directive(self.event, payload, exception=True)

    # ----------------------------------------------------------------------------------------------------------------
    def make_http_error(self, err, response=None):
        payload = {
            'error': {
                'type': 'Error',
                'message': 'fanout generic error',
            },
            'details': {
                'scope': 'fanout',
                'code': str(err.code),
                'text': err.text(),
            },
        }

        if response:
            payload['Response'] = response

        return self.make_directive(self.event, payload, exception=True)

    # ----------------------------------------------------------------------------------------------------------------
    def make_uniproxy_error(self, err):
        payload = {
            'error': {
                'type': 'Error',
                'message': 'uniproxy error',
            },
            'details': {
                'scope': 'uniproxy',
                'code': 'generic',
                'text': str(err)
            }
        }
        return self.make_directive(self.event, payload, exception=True)

    # ----------------------------------------------------------------------------------------------------------------
    def make_asr_error(self, err):
        payload = {
            'error': {
                'type': 'Error',
                'message': 'recognition error',
            },
            'details': {
                'scope': 'asr-server',
                'code': 'generic',
                'text': str(err)
            }
        }
        return self.make_directive(self.event, payload, exception=True)


# ====================================================================================================================

def with_fx_metrics(templ):
    fx_codes = ("200", "4xx", "5xx", "597", "598", "599")

    def add_metrics(cls):
        for code in fx_codes:
            val = templ.format(code) if (templ is not None) else None
            setattr(cls, "F{}".format(code), val)
        return cls

    return add_metrics


@with_fx_metrics(None)
class NullMetrics:
    Recv = None
    Fail = None
    Done = None
    Time = None
    Current = None
    FOther = None


# ====================================================================================================================
class MessengerProcessor(MessengerProcessorBase):
    def __init__(self, *args, **kwargs):
        self._logger = Logger.get(kwargs.pop('logger_name', 'mssngr.event'))

        self._request_type = kwargs.pop('request_type')
        self._request_url = kwargs.pop('request_url')
        self._response_type = kwargs.pop('response_type')
        self._response_name = kwargs.pop('response_name')
        self._custom_response = kwargs.pop('custom_response', False)
        self._client = kwargs.pop('mssngr_client')
        self._metrics = kwargs.pop('metrics', NullMetrics)
        self._accept_gzip = kwargs.pop('accept_gzip', False)
        self._failed = False
        if self._client is None:
            self._logger.debug('custom _client is None, getting default instance')
            c = mssngr_client_instance()
            self._logger.debug('default instance is {}'.format(c))
            self._client = c

        super(MessengerProcessor, self).__init__(*args, **kwargs)

    # ----------------------------------------------------------------------------------------------------------------
    @property
    def _ioloop(self) -> tornado.ioloop.IOLoop:
        return tornado.ioloop.IOLoop.current()

    # ----------------------------------------------------------------------------------------------------------------
    def make_response(self, response):
        pass

    # ----------------------------------------------------------------------------------------------------------------
    def process_event(self, event):
        super().process_event(event)
        self._ioloop.spawn_callback(self.co_process_event, event)

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def perform_request(self, event):
        ok = False
        directive = None

        try:
            with UnistatTiming(self._metrics.Time):
                ok, response = yield self._client.fetch(
                    self._request_url,
                    self._request_type,
                    self._response_type,
                    event.payload,
                    accept_gzip=self._accept_gzip
                )

            if self._custom_response:
                response = self.make_response(event.payload, response)

            if response is not None:
                directive = self.make_directive(event, response, not ok)
            else:
                directive = None

        except AuthenticationError as ex:   # no auth
            self.on_fail(auth=True)
            self._logger.exception(ex)
            directive = self.make_auth_error(ex)

        except MessengerError as ex:        # protobuf content in response
            self._logger.exception(ex)
            directive = self.make_messenger_error(ex)
            self.on_fail()
            self.on_status(ex.code)

        except HTTPError as ex:             # plain content in response
            self._logger.exception(ex)
            directive = self.make_http_error(ex)
            self.on_fail()
            self.on_status(ex.code)

        except Exception as ex:             # generic (non-fanout) error
            self._logger.exception(ex)
            directive = self.make_uniproxy_error(ex)
            self.on_fail()

        self._logger.debug('ok={} directive={}'.format(ok, directive))

        return ok, directive

    # ----------------------------------------------------------------------------------------------------------------
    def on_recv(self):
        if self._metrics.Recv:
            GlobalCounter.g_counters[self._metrics.Recv].increment()

        if self._metrics.Current:
            GlobalCounter.g_counters[self._metrics.Current].increment()

    # ----------------------------------------------------------------------------------------------------------------
    def on_fail(self, auth=False):
        self._failed = True

        if self._metrics.Fail:
            GlobalCounter.g_counters[self._metrics.Fail].increment()

        if self._metrics.Current:
            GlobalCounter.g_counters[self._metrics.Current].decrement()

        if auth:
            self.on_auth_error()

    # ----------------------------------------------------------------------------------------------------------------
    def on_auth_error(self):
        if self._metrics.FNoAuth:
            GlobalCounter.g_counters[self._metrics.FNoAuth].increment()

    # ----------------------------------------------------------------------------------------------------------------
    def on_status(self, status):
        code = status // 100
        if HTTPError.is_internal(status):
            metric = getattr(self._metrics, "F{}".format(status), None)
            if metric:
                GlobalCounter.g_counters[metric].increment()
        elif code == 4 and self._metrics.F4xx is not None:
            GlobalCounter.g_counters[self._metrics.F4xx].increment()
        elif code == 5 and self._metrics.F5xx is not None:
            GlobalCounter.g_counters[self._metrics.F5xx].increment()
        elif self._metrics.FOther is not None:
            GlobalCounter.g_counters[self._metrics.FOther].increment()

    # ----------------------------------------------------------------------------------------------------------------
    def on_done(self):
        if self._failed:
            return

        if self._metrics.Done:
            GlobalCounter.g_counters[self._metrics.Done].increment()

        if self._metrics.F200:
            GlobalCounter.g_counters[self._metrics.F200].increment()

        if self._metrics.Current:
            GlobalCounter.g_counters[self._metrics.Current].decrement()

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def co_process_event(self, event):
        try:
            self.on_recv()

            yield self.wait_for_auth()

            ok, directive = yield self.perform_request(event)

        except AuthenticationError as ex:   # no auth
            directive = self.make_auth_error(ex)
            self.on_fail(auth=True)

        except Exception as ex:             # generic (non-fanout) error
            directive = self.make_uniproxy_error(ex)
            self.on_fail()

        if directive is not None:
            self.system.write_directive(directive)
        else:
            pass    # log it or delete it?

        self.on_done()

        self.close()


# ====================================================================================================================
@with_fx_metrics("mssngr_messages_in_fanout_{}_summ")
class PostMessageMetrics:
    Recv = 'mssngr_messages_in_recv_summ'
    Fail = 'mssngr_messages_in_fail_summ'
    Done = 'mssngr_messages_in_push_summ'
    Time = 'mssngr_in_d_wait'
    Current = 'mssngr_running_push_ammx'
    FOther = 'mssngr_messages_in_fanout_other_summ'
    FNoAuth = 'mssngr_messages_in_unauthorized_summ'


# ====================================================================================================================
@register_event_processor
class PostMessage(MessengerProcessor):
    def __init__(self, *args, **kwargs):
        super().__init__(
            *args,
            request_type=TInMessage,
            request_url='/push',
            response_type=TResponse,
            response_name='Ack',
            custom_response=True,
            logger_name=kwargs.get('logger_name', 'mssngr.post'),
            mssngr_client=kwargs.get('mssngr_client'),
            metrics=PostMessageMetrics
        )

    # ----------------------------------------------------------------------------------------------------------------
    def make_response(self, payload, response):
        payload_id = payload.get(
            'PayloadId',
            payload.get('ClientMessage', {}).get('Plain', {}).get('PayloadId')
        )

        if self.system.mssngr_version is None or self.system.mssngr_version < 4:
            client_message = payload.get('ClientMessage', {})
            ack_required = False
            ack_required |= 'BotRequest' in client_message
            ack_required |= 'Plain' in client_message
            ack_required |= 'CallingMessage' in client_message
            ack_required |= 'SeenMarker' in client_message
            ack_required |= 'Report' in client_message
            ack_required |= 'Pin' in client_message
            ack_required |= 'Unpin' in client_message
            ack_required |= 'ChatApproval' in client_message
            ack_required |= 'ReadMarker' in client_message
            ack_required |= 'PayloadId' in payload
        else:
            ack_required = True

        result = None
        if ack_required and payload_id:
            result = {
                'Ack': payload_id,
                'TsEcR': payload.get('TsVal', 0),
                'TsVal': int(time.time() * 1000),
            }
        elif ack_required:
            result = {
                'TsEcR': payload.get('TsVal', 0),
                'TsVal': int(time.time() * 1000),
            }

        if result and response:
            result['Response'] = response

        return result


# ====================================================================================================================
@with_fx_metrics("mssngr_history_in_fanout_{}_summ")
class HistoryRequestMetrics:
    Recv = 'mssngr_history_in_recv_summ'
    Fail = 'mssngr_history_in_fail_summ'
    Done = 'mssngr_history_in_got_summ'
    Time = 'mssngr_history_time'
    Current = 'mssngr_running_push_ammx'
    FOther = 'mssngr_history_in_fanout_other_summ'
    FNoAuth = 'mssngr_history_in_unauthorized_summ'


# ====================================================================================================================
@register_event_processor
class HistoryRequest(MessengerProcessor):
    def __init__(self, *args, **kwargs):
        super().__init__(
            *args,
            request_type=THistoryRequest,
            request_url='/history',
            response_type=THistoryResponse,
            response_name='History',
            logger_name='mssngr.history',
            metrics=HistoryRequestMetrics,
            mssngr_client=kwargs.get('mssngr_client'),
            accept_gzip=True
        )


# ====================================================================================================================
@with_fx_metrics("mssngr_edit_history_fanout_{}_summ")
class EditHistoryRequestMetrics:
    Recv = 'mssngr_edit_history_recv_summ'
    Fail = 'mssngr_edit_history_fail_summ'
    Done = 'mssngr_edit_history_got_summ'
    Time = 'mssngr_edit_history_time'
    Current = 'mssngr_running_push_ammx'
    FOther = 'mssngr_edit_history_fanout_other_summ'
    FNoAuth = 'mssngr_edit_history_unauthorized_summ'


# ====================================================================================================================
@register_event_processor
class EditHistoryRequest(MessengerProcessor):
    def __init__(self, *args, **kwargs):
        super().__init__(
            *args,
            request_type=TEditHistoryRequest,
            request_url='/edit_history',
            response_type=TEditHistoryResponse,
            response_name='EditHistoryResponse',
            logger_name='mssngr.edit_history',
            metrics=EditHistoryRequestMetrics,
            mssngr_client=kwargs.get('mssngr_client')
        )


# ====================================================================================================================
@with_fx_metrics("mssngr_subscription_in_fanout_{}_summ")
class SubscriptionRequestMetrics:
    Recv = 'mssngr_subscription_in_recv_summ'
    Fail = 'mssngr_subscription_in_fail_summ'
    Done = 'mssngr_subscription_in_got_summ'
    Time = 'mssngr_subscribe_time'
    Current = 'mssngr_running_push_ammx'
    FOther = 'mssngr_subscription_in_fanout_other_summ'
    FNoAuth = 'mssngr_subscription_in_unauthorized_summ'


# ====================================================================================================================
@register_event_processor
class SubscriptionRequest(MessengerProcessor):
    def __init__(self, *args, **kwargs):
        super().__init__(
            *args,
            request_type=TSubscriptionRequest,
            request_url='/subscribe',
            response_type=TSubscriptionResponse,
            response_name='SubscriptionResponse',
            logger_name='mssngr.subscribe',
            metrics=SubscriptionRequestMetrics,
            mssngr_client=kwargs.get('mssngr_client')
        )


# ====================================================================================================================
@with_fx_metrics("mssngr_whoami_in_fanout_{}_summ")
class WhoamiRequestMetrics:
    Recv = 'mssngr_whoami_in_recv_summ'
    Fail = 'mssngr_whoami_in_fail_summ'
    Done = 'mssngr_whoami_in_got_summ'
    Time = 'mssngr_whoami_time'
    Current = None
    FOther = 'mssngr_whoami_in_fanout_other_summ'
    FNoAuth = 'mssngr_whoami_in_unauthorized_summ'


# ====================================================================================================================
@register_event_processor
class Whoami(MessengerProcessor):
    def __init__(self, *args, **kwargs):
        super().__init__(
            *args,
            request_type=TWhoamiRequest,
            request_url='/whoami',
            response_type=TWhoamiResponse,
            response_name='WhoamiResponse',
            logger_name='mssngr.whoami',
            metrics=WhoamiRequestMetrics,
            mssngr_client=kwargs.get('mssngr_client')
        )


# ====================================================================================================================
@with_fx_metrics("mssngr_minfo_in_fanout_{}_summ")
class MessageInfoMetrics:
    Recv = 'mssngr_minfo_in_recv_summ'
    Fail = 'mssngr_minfo_in_fail_summ'
    Done = 'mssngr_minfo_in_got_summ'
    Time = 'mssngr_minfo_time'
    Current = None
    FOther = 'mssngr_minfo_in_fanout_other_summ'
    FNoAuth = 'mssngr_minfo_in_unauthorized_summ'


# ====================================================================================================================
@register_event_processor
class MessageInfoRequest(MessengerProcessor):
    def __init__(self, *args, **kwargs):
        super().__init__(
            *args,
            **kwargs,
            request_type=TMessageInfoRequest,
            request_url='/message_info',
            response_type=TMessageInfoResponse,
            response_name='MessageInfo',
            logger_name='mssngr.minfo',
            metrics=MessageInfoMetrics,
            mssngr_client=kwargs.get('mssngr_client')
        )


# ====================================================================================================================
@register_event_processor
class VoiceInput(PostMessage):
    _close_required = False

    # ----------------------------------------------------------------------------------------------------------------
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        # Vins.VoiceInput:
        self.data_size = 0
        self.first_logger = AccessLogger('asr_first_chunk', self.system, rt_log=self.rt_log)
        self.full_logger = AccessLogger('asr_full', self.system, rt_log=self.rt_log)
        self.asr_stream = None
        self.asr_result_future = tornado.concurrent.Future()

    # ----------------------------------------------------------------------------------------------------------------
    def check_event_payload(self, event):
        pass

    # ----------------------------------------------------------------------------------------------------------------
    @tornado.gen.coroutine
    def co_process_event(self, event):
        directive = None

        try:
            yield self.wait_for_auth()

            self.check_event_payload(event)

            self.asr_stream = YaldiStream(
                self._on_asr_result,
                self._on_asr_error,
                self.payload_with_session_data,
                self.system.session_id,
                self.event.message_id,
                host=self.system.srcrwr['ASR'],
                port=self.system.srcrwr.ports.get('ASR'),
                rt_log=self.rt_log
            )

            text = yield self.asr_result_future

            event.payload['ClientMessage']['Plain']['Text']['MessageText'] = text

            _, directive = yield self.perform_request(event)

        except AuthenticationError as ex:   # no auth
            directive = self.make_auth_error(ex)
            self.on_fail(auth=True)

        except Exception as ex:
            directive = self.make_uniproxy_error(ex)
            self.on_fail()

        if directive is not None:
            self.system.write_directive(directive)

        self.close()

    # ----------------------------------------------------------------------------------------------------------------
    def add_data(self, data):
        self.data_size += len(data)

        if self.first_logger:
            self.first_logger.end(code=200, size=self.data_size)
            self.first_logger = None

        self.asr_stream.add_chunk(data)

    # ----------------------------------------------------------------------------------------------------------------
    def process_streamcontrol(self, _):
        self.asr_stream.add_chunk()

    # ----------------------------------------------------------------------------------------------------------------
    def _finalize_asr_stream(self):
        if self.full_logger:
            self.full_logger.end(code=200, size=self.data_size)
            self.full_logger = None
        self.asr_stream.close()
        self.system.close_stream(self.event.stream_id)

    # ----------------------------------------------------------------------------------------------------------------
    def _on_asr_error(self, err):
        if self.full_logger:
            self.full_logger.end(code=500, size=self.data_size)
            self.full_logger = None

        self.asr_result_future.set_exception(
            Exception(err)
        )

    # ----------------------------------------------------------------------------------------------------------------
    def _on_asr_result(self, result):
        Recognize.on_result_asr(self, result)

        if not result.get('endOfUtt'):
            return

        self._finalize_asr_stream()

        if not result.get('recognition'):
            self.asr_result_future.set_exception(
                Exception('Empty message was recognized but not supported!')
            )
            return

        text_input = result['recognition'][0]['normalized']

        self.asr_result_future.set_result(text_input)


# ====================================================================================================================
@register_event_processor
class SetVoiceChats(EventProcessor):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._log = Logger.get('mssngr.set_voice_chats')

    # ----------------------------------------------------------------------------------------------------------------
    def process_event(self, event):
        super(SetVoiceChats, self).process_event(event)
        voice_chat_ids = self.event.payload.get('ChatIds', [])
        self.system.messenger_voice_chats = voice_chat_ids
        self._log.debug('set voice chats for {}: {}'.format(self.system.uuid(), voice_chat_ids))
        GlobalCounter.MSSNGR_SET_VOICE_CHATS_IN_RECV_SUMM.increment()


# ====================================================================================================================
@register_event_processor
class Fanout(EventProcessor):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._log = Logger.get('mssngr.fanout')

    # ----------------------------------------------------------------------------------------------------------------
    def is_event_exception(self, client_message):
        return 'EventException' in client_message

    # ----------------------------------------------------------------------------------------------------------------
    def is_voice_message(self, client_message):
        if 'Plain' not in client_message:
            return False

        chat_id = client_message['Plain'].get('ChatId')
        if chat_id not in self.system.messenger_voice_chats:
            return False

        return True

    # ----------------------------------------------------------------------------------------------------------------
    def send_event_exception(self, client_message):
        event_exception = client_message['EventException']
        event_exception_details = {k.lower(): v for k, v in event_exception['Details'].items()}
        self.dispatch_error_ex(event_exception['Message'], event_exception_details, close=False)

    # ----------------------------------------------------------------------------------------------------------------
    def send_message(self, data):
        directive = Directive('Messenger', 'Message', data)
        message = directive.create_message(self.system)
        self.system.write_message(message)
        return message['directive']['header']['messageId']

    # ----------------------------------------------------------------------------------------------------------------
    def process_event(self, event):
        super().process_event(event)

        data = event.payload['data']
        try:
            client_message = data.get('ServerMessage', {}).get('ClientMessage', {})

            if self.is_event_exception(client_message):
                self.send_event_exception(client_message)
                return

            message_id = self.send_message(data)

            if self.is_voice_message(client_message):
                self.voice_message(client_message, message_id)
        except Exception as exc:
            self._log.exception('fanout fail {}: ({})'.format(exc, data))
        finally:
            self.close()

    # ----------------------------------------------------------------------------------------------------------------
    def voice_message(self, client_message, message_id):
        text = client_message['Plain']['Text']['MessageText']

        payload = {
            'format': 'Opus',
            'lang': 'ru-RU',
            'quality': 'UltraHigh',
            'text': text,
            'voice': 'shitova.gpu',
        }
        payload.update(self.payload_with_session_data)

        self.system.process_event(Event({
            'header': {
                'name': 'Generate',
                'namespace': 'TTS',
                # We use the same message id to bind all the results with the same refMessageId
                'messageId': message_id,
                'refStreamId': self.event.ref_stream_id,
            },
            'payload': payload,
        }))
