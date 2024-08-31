import re
import weakref
import time
import alice.uniproxy.library.utils.deepcopyx as copy
import datetime

from rtlog import null_logger
from tornado.ioloop import IOLoop
from tornado import gen

from alice.uniproxy.library.settings import config
from alice.uniproxy.library.settings import TOPIC_MAPS
from alice.uniproxy.library.settings import LANG_MAPS

from . import GaldiStream
from .asr_json_adapter import convert_init_request_asr0_to_asr1
from alice.uniproxy.library.events import Directive
from alice.uniproxy.library.backends_common.protostream import ProtoStream
from alice.uniproxy.library.backends_common.protohelpers import proto_from_json, proto_to_json

from voicetech.library.proto_api.yaldi_pb2 import InitRequest, InitResponse, AddData, AddDataResponse

from alice.uniproxy.library.logging import Logger
from alice.uniproxy.library.utils import deepupdate, conducting_experiment, experiment_value

from alice.uniproxy.library.global_counter import GlobalCounter, GlobalTimings
from alice.uniproxy.library.global_counter import EC_CLIENT_INACTIVITY_TIMEOUT
from alice.uniproxy.library.global_counter import EC_SERVER_INACTIVITY_TIMEOUT
from alice.uniproxy.library.global_counter import EC_CANCEL
from alice.uniproxy.library.global_counter import Unistat
from alice.uniproxy.library.global_counter.uniproxy import ASR_MERGE_HGRAM
from alice.uniproxy.library.global_counter.uniproxy import ASR_MERGE_FIRST_RESPONSE_HGRAM
from alice.uniproxy.library.global_counter.uniproxy import ASR_MERGE_SECOND_RESPONSE_HGRAM

from alice.uniproxy.library.global_state import GlobalTags


def _ensure_normalized_recognition(recognition):
    for hyp in recognition:
        if not hyp["normalized"]:
            hyp["normalized"] = " ".join((x.get("value", "") for x in hyp.get("words", [])))


def _is_robot_uuid(system):
    uuid = system.uuid()
    if not uuid:
        return False
    u = Unistat(system)
    u.check_uuid(uuid)
    return u.is_robot


def get_asr_srcrwr_name(lang, model):
    return '_'.join(['ASR', lang[:2], model.replace('-', '_')]).upper()


class YaldiProto(ProtoStream):
    def __init__(self, yaldi_stream, asr_host, asr_port, url_path, service_name,
                 rt_log=null_logger(), rt_log_label=None, use_balancing_hint=False, balancer_mode='pre_prod'):
        super(YaldiProto, self).__init__(asr_host, asr_port, url_path, rt_log, rt_log_label)
        self._log = Logger.get('.yaldiproto')
        self.yaldi_stream = yaldi_stream
        self.upgrade_resp = None
        self._service_name = service_name
        if use_balancing_hint:
            self.balancing_hint_header = GlobalTags.get_balancing_hint_header(balancer_mode)

    def on_connect(self, duration):
        self.yaldi_stream.on_connect(duration)

    def process(self):
        self.yaldi_stream.send_init_request()

    def on_fail_http_upgrade(self, http_resp):
        self.yaldi_stream.on_fail_http_upgrade(http_resp)

    def on_error(self, message, code):
        self.yaldi_stream.on_error(message, code=code)

    def service_name(self):
        return self._service_name

    def is_closed(self):
        if self._closed:
            return True
        if self.stream is not None and self.stream.closed():
            self.close()
            return True
        return False

    def is_closed_2(self):
        if self._closed:
            return True, 'YaldiProto is closed'
        if self.stream is not None and self.stream.closed():
            self.close()
            return True, 'YaldiProto.stream is closed'
        return False, ''

    def on_send_protobuf(self, result):
        """need for call as callback from send_protobuf"""
        if not result:
            self.close()

    def soft_close(self):
        """method for abort connection without increment errors counters"""
        IOLoop.current().spawn_callback(self._soft_close)

    @gen.coroutine
    def _soft_close(self):
        if not self.is_closed():
            if not self.is_connected():
                self.close()
                return

            # send force closing message (closeConnection=True)
            # read all leftovers from socket & close it
            try:
                data = AddData(lastChunk=True, closeConnection=True)
                self.send_protobuf(data, self.on_send_protobuf)
                handshake_timelimit = 7
                while self.stream and self.stream.reading():
                    # has coroutine which already read from stream
                    handshake_timelimit -= 1
                    yield gen.sleep(1)
                if self.stream and handshake_timelimit:
                    yield gen.with_timeout(
                        datetime.timedelta(seconds=handshake_timelimit),
                        self.stream.read_until_close(),
                    )
                self.close()
            except (TimeoutError, gen.TimeoutError) as exc:
                self._log.warning('timeout on _soft_close(): {}'.format(exc))
                self.close()
            except Exception:
                self._log.exception('unexpected error on _soft_close()')
                self.close()


class InactivityTimer:
    def __init__(self, timeout, callback):
        self.timeout = timeout
        self.callback = callback
        self.on_activity()
        if timeout is None:
            self.timer = None
        else:
            self.timer = IOLoop.current().call_later(self.timeout, self.on_timeout)

    def on_activity(self):
        self.activity_ts = time.time()

    def on_timeout(self):
        inactivity_ts = time.time()
        next_timeout = (self.activity_ts + self.timeout) - inactivity_ts
        if next_timeout > 0:
            self.timer = IOLoop.current().call_later(next_timeout, self.on_timeout)
        else:
            self.callback()

    def cancel(self):
        if self.timer is not None:
            IOLoop.current().remove_timeout(self.timer)
            self.timer = None


class YaldiStat:
    def __init__(self, topic):
        self._topic = None
        self._setup_counters(topic)  # set self._topic as well
        self.normalized_words = None
        self.num_equal = 0
        self.prev_timestamp = None
        self.first_eq_timestamp = None
        self._result_codes = []

    def _setup_counters(self, topic):
        topic = topic.replace('-', '_')
        allowed_topics_aliases = {
            'dialogeneral': 'dialoggeneral',
            'dialog_general_gpu': 'dialoggeneral_gpu',
            'desktopgeneral': 'desktopgeneral',
            'quasar_general': 'quasargeneral',
            'quasar_general_gpu': 'quasargeneral_gpu',
            'dialogmaps': 'dialogmaps',
            'dialogmapsgpu': 'dialogmaps_gpu'
        }
        if topic not in allowed_topics_aliases:
            self.enable = False
            return
        self._topic = allowed_topics_aliases[topic]
        self.enable = True
        self.eq_counter_name = ('asr_partial_eq_' + self._topic + '_summ').upper()
        self.eq_timing_name = ('asr_eou_eq_time_' + self._topic).lower()
        self.ne_timing_name = ('asr_eou_ne_time_' + self._topic).lower()
        self.partials_timing_name = ('asr_{}_partials'.format(self._topic)).lower()

    def _get_normalized_words(self, res):
        recognition = res.get('recognition', [])
        if len(recognition) == 0:
            return ''
        words_in_recognition = recognition[0].get('words', [])
        words = []
        for word in words_in_recognition:
            words.append(word.get('value', '').lower())
        if len(words) == 0:
            return ''
        return '|'.join(words)

    def add_result_code(self, code):
        if code is not None:
            self._result_codes.append(code)

    def finish(self):
        def count_code(topic, code):
            if code == EC_CLIENT_INACTIVITY_TIMEOUT:
                signame = "ASR_{}_CLIENT_TIMEOUT_SUMM"
            elif code == EC_SERVER_INACTIVITY_TIMEOUT:
                signame = "ASR_{}_SERVER_TIMEOUT_SUMM"
            elif code == 200:
                signame = "ASR_{}_200_SUMM"
            elif code // 100 == 5:
                signame = "ASR_{}_5XX_SUMM"
            else:
                return
            getattr(GlobalCounter, signame.format(topic)).increment()

        if not (self._topic and self._result_codes):
            return

        topic = self._topic.upper()
        count_code(topic, self._result_codes[0])
        if len(self._result_codes) > 1:
            count_code(topic + "_PUMPKIN", self._result_codes[-1])

    def process_new_data(self, res):
        if not self.enable:
            return

        timestamp = time.monotonic()
        words = self._get_normalized_words(res)

        if self.normalized_words is None:
            self.normalized_words = words
            self.num_equal = 1
            self.prev_timestamp = timestamp
            self.first_eq_timestamp = timestamp
            return

        if self.normalized_words == words:
            self.num_equal += 1
        else:
            self.normalized_words = words
            self.num_equal = 1
            self.first_eq_timestamp = timestamp

        if res.get('endOfUtt', False):
            if self.num_equal == 1:
                GlobalTimings.store(self.ne_timing_name, timestamp - self.prev_timestamp)
            else:
                GlobalTimings.store(self.eq_timing_name, timestamp - self.first_eq_timestamp)
                getattr(GlobalCounter, self.eq_counter_name).increment()
            self.normalized_words = None
            self.num_equal = 0
            self.prev_timestamp = None
            self.first_eq_timestamp = None
        else:
            GlobalTimings.store(self.partials_timing_name, timestamp - self.prev_timestamp)
            self.prev_timestamp = timestamp


class YaldiStreamOptions:
    def __init__(self, params, flags):
        self.wait_after_first_eou = self.get_wait_after_eou(params, flags)
        self.ignore_trash_results = self.get_ignore_trash_results(params, flags)
        self.is_suggester_enabled = self.get_suggester_enabled(params, flags)

    def get_wait_after_eou(self, params, flags):
        if not conducting_experiment('e2e_wait_after_eou', params):
            return None

        if 'wait_after_first_eou' not in params:
            return None

        try:
            delay = float(params['wait_after_first_eou'])
        except Exception:
            delay = 1.0

        return delay

    def get_ignore_trash_results(self, params, flags):
        return conducting_experiment('ignore_trash_classified_results', params, flags)

    def get_suggester_enabled(self, params, flags):
        if params.get('settings_from_manager', {}).get('asr_enable_suggester', False):
            return True
        if conducting_experiment('asr_enable_suggester', params, flags):
            return True
        return False


class YaldiStream:
    default_json = {
        "hostName": "localhost",
        "requestId": "123",
        "uuid": "123",
        "device": "uniproxy",
        "coords": "0,0",
        "topic": "dialogeneral",
        "lang": "ru-RU",
        "sampleRate": "16000",  # TODO: remove (need update yaldi proto)
    }

    def __init__(
        self,
        callback, error_callback, params, session_id, message_id,
        close_callback=None, host=None, port=None, unistat_counter='yaldi',
        rt_log=null_logger(), rt_log_label=None, system=None, context_futures=(),
        apphosted_asr=False, error_ex_cb=None, is_spotter=False, contacts_future=None
    ):
        self.log_info = {}
        self._apphosted_asr = apphosted_asr
        self._log = Logger.get('.yaldistream')
        self._context_futures = context_futures
        # note: it could be rewrote after mapping lang & model
        self.yaldi_host = host or params.get('asr_balancer') or config['asr']['yaldi_host']
        self.yaldi_port = port or config['asr']['yaldi_port']
        self.yaldi_proto = None
        self.callback = weakref.WeakMethod(callback)
        self.error_callback = weakref.WeakMethod(error_callback)
        self.error_ex_callback = weakref.WeakMethod(error_ex_cb) if error_ex_cb else None
        if close_callback:
            self.close_callback = weakref.WeakMethod(close_callback)
        else:
            self.close_callback = None
        self.unistat_counter = unistat_counter
        self._system = system
        self.params = deepupdate(YaldiStream.default_json, params, copy=True)
        self.options = YaldiStreamOptions(self.params, self._system.uaas_flags if self._system else None)
        if system:
            self.params['clientHostname'] = system.hostname
            if system.uaas_asr_flags:
                self.params['experiments'] = system.uaas_asr_flags
        self.params["requestId"] = message_id
        self.session_id = '{} {}:'.format(session_id, unistat_counter)
        self.message_id = message_id
        self.rt_log = rt_log
        self.rt_log_label = rt_log_label
        self.http_upgrade_error = None  # can contain 404
        self.init_sent = False
        self.reading = False
        self.last_chunk = False
        self.has_finish_code = False
        self._ignore_add = False

        self.client_ip = None
        self.app_id = None
        self.is_spotter = is_spotter

        if system:
            self.client_ip = system.client_ip
            self.app_id = system.app_id

        # VOICESERV-2017 debug
        self.receive_end_of_utt = False
        self.time_last_send_protobuf = None
        self.time_last_recv_protobuf = None

        # ###################### KILLMEPLS: begin #####################
        # fix params backward compatibility
        self.mapped_lang = LANG_MAPS.get(self.params['lang'])
        self.mapped_topic = TOPIC_MAPS.get(self.params['topic'], self.params['lang'])

        self.params['lang'] = self.mapped_lang

        convert_init_request_asr0_to_asr1(self.params)
        # ###################### KILLMEPLS: end   #####################

        if system:
            srcrwr_name = get_asr_srcrwr_name(self.mapped_lang, self.mapped_topic)
            host = system.srcrwr[srcrwr_name]
            if host:
                self.yaldi_host = host

            port = system.srcrwr.ports.get(srcrwr_name)
            if port:
                self.yaldi_port = port

        self._yaldi_stat = YaldiStat(self.mapped_topic)
        self.processed = 0
        self.sent = 0
        self.buffered = []

        if self._apphosted_asr:
            self.client_inactivity_timer = None
            self.yaldi_inactivity_timer = None
        else:
            # client & yaldi activity timeouts
            def on_client_inactivity_timeout():
                self.on_error('YaldiStream reach client inactivity time limit', code=EC_CLIENT_INACTIVITY_TIMEOUT)

            self.client_inactivity_timer = InactivityTimer(
                self.params.get("asr_client_inactivity_timeout", config['asr'].get('client_inactivity_timeout')),
                on_client_inactivity_timeout,
            )

            def on_yaldi_inactivity_timeout():
                self.on_error('YaldiStream reach yaldi inactivity time limit', code=EC_SERVER_INACTIVITY_TIMEOUT)

            self.yaldi_inactivity_timer = InactivityTimer(
                config['asr'].get('yaldi_inactivity_timeout'),
                on_yaldi_inactivity_timeout,
            )

        if self._system:
            advanced_options = self.params['advanced_options']

            # VOICESERV-3351
            # WARNING: degradation_mode asr-servers with version < 52 may cause segfault
            # by default degradation is `Disable` for robots and `Auto` for real users
            advanced_options.setdefault("degradation_mode", "Disable" if _is_robot_uuid(self._system) else "Auto")

            # VOICESERV-2315
            if conducting_experiment('use_trash_talk_classifier', self.params, self._system.uaas_flags):
                advanced_options['use_trash_talk_classifier'] = True
            # VOICESERV-2353
            if conducting_experiment('enable_e2e_eou', self.params, self._system.uaas_flags):
                advanced_options['enable_e2e_eou'] = True
            # VOICESERV-2673
            partial_update_period = experiment_value('partial_update_period', self.params, self._system.uaas_flags)
            if partial_update_period is not None:
                try:
                    advanced_options['partial_update_period'] = int(partial_update_period)
                except Exception:
                    self.WARN('ignore invalid partial_update_period={}'.format(partial_update_period))

            # VOICESERV-4028
            if self.options.is_suggester_enabled:
                advanced_options['enable_suggester'] = True

        self.fallback_topics = []
        if "disable_fallback" not in self.params:
            if 'backup_host' in config['asr']:
                self.fallback_topics.append((config['asr']['backup_host'], self.params['topic']))

            if 'pumpkin_host' in config['asr']:
                self.fallback_topics.append((config['asr']['pumpkin_host'], 'dialogeneralfast'))

            if not self.fallback_topics:
                self.fallback_topics = [(self.yaldi_host, 'dialogeneralfast')]

        # this abomination is a memorial of people's will to make stupid changes to perfect architecture
        self.yet_to_connect = True

        if contacts_future is not None:
            contacts_future.add_done_callback(self.connect_with_contacts)
        elif context_futures:
            gen.multi(context_futures).add_done_callback(self.connect_with_contexts)
        else:
            self.connect()

    def _get_url_path(self):
        url_path = '/{}/{}/'.format(self.mapped_lang.lower(), self.mapped_topic.lower())
        if self.mapped_lang.lower() == 'tr-tr' and self.mapped_topic.lower() == 'dialogmapsgpu':
            url_path = '/ru-ru/dialogmapsgpu/'

        # VOICE-6259
        if not self.is_spotter and self._system and conducting_experiment('asr_quasar_monolith', self.params, self._system.uaas_flags):
            url_path = '/ru-ru/quasar-general-monolith/'
        return url_path

    def connect(self):
        self.yet_to_connect = False

        url_path = self._get_url_path()

        self.DLOG('for topic={} lang={} use yaldi balancer url_path=proto://{}:{}/{}'.format(
            self.params['topic'], self.params['lang'], self.yaldi_host, self.yaldi_port, url_path))
        rt_log_label = '{}: {}'.format(self.rt_log_label if self.rt_log_label else '?asr?', url_path)
        balancing_mode_key = {
            'dialog-general-gpu': 'balancing_mode_asr_dialog',
            'quasar-general-gpu': 'balancing_mode_asr_quasar',
            'dialogmapsgpu': 'balancing_mode_asr_dialogmaps'
        }.get(self.mapped_topic.lower(), None)
        self.yaldi_proto = YaldiProto(
            self, self.yaldi_host, self.yaldi_port,
            url_path, self.unistat_counter,
            rt_log=self.rt_log,
            rt_log_label=rt_log_label,
            use_balancing_hint=self._system.use_balancing_hint if self._system else False,
            balancer_mode=self.params.get('settings_from_manager', {}).get(balancing_mode_key, 'pre_prod')
        )
        self.yaldi_proto.message_id = self.params["requestId"]
        if not self._apphosted_asr:
            self.yaldi_proto.connect()

    def connect_with_contacts(self, contacts):
        try:
            self.params['user_info'] = {
                'ContactBookItems': [{'DisplayName': x['display_name']} for x in contacts.result()['data']['contacts']]
            }
        except Exception as err:
            self.ERR('context error:', err)
        if self._context_futures:
            gen.multi(self._context_futures).add_done_callback(self.connect_with_contexts)
        else:
            self.connect()

    def connect_with_contexts(self, contexts):
        try:
            self.params['context'] = [context for context in contexts.result() if context is not None]
        except Exception as err:
            self.ERR('context error:', err)
        self.connect()

    def on_connect(self, duration):
        GlobalTimings.store(self.unistat_counter + '_connect_time', duration)

    def on_fail_http_upgrade(self, http_resp):
        try:
            if http_resp:
                # fast & dirty method 404 errors checking
                # if yaldi balancer error is not 404 can try fallback
                tk = http_resp.split(b'\n')
                if len(tk) and b'HTTP/1.1 404' in tk[0]:
                    self.INFO('got http code=404')
                    self.http_upgrade_error = 404
                    return
        except Exception:
            pass
        self.http_upgrade_error = 666  # some fake error code (number != 404 and != 0)

    def close(self, farewell_handshake=True):
        self.DLOG('close')
        self.increment_unistat_counter(EC_CANCEL)
        if self.client_inactivity_timer:
            self.client_inactivity_timer.cancel()
            self.client_inactivity_timer = None
        if self.yaldi_inactivity_timer:
            self.yaldi_inactivity_timer.cancel()
            self.yaldi_inactivity_timer = None
        self.callback = None
        self.error_callback = None
        self.error_ex_callback = None
        self.buffered = []
        if self.yaldi_proto:
            yaldi_proto = self.yaldi_proto
            self.yaldi_proto = None
            if farewell_handshake:
                yaldi_proto.soft_close()
            else:
                # already got last/final message from backend
                yaldi_proto.close()
        if self.close_callback is not None:
            cb = self.close_callback()
            self.close_callback = None
            if cb:
                cb()

    def is_closed(self):
        if self.yet_to_connect:
            return False

        if self.yaldi_proto is None:
            return True

        return self.yaldi_proto.is_closed()

    def is_closed_2(self):
        if self.yet_to_connect:
            return False, ''

        if self.yaldi_proto is None:
            return True, 'YaldiProto is None'

        return self.yaldi_proto.is_closed_2()

    def on_error(self, message, code=None, details=None):
        if code:
            self.increment_unistat_counter(code)
        if code is not None and code == 599:  # 599 == can not connect or can not read response
            self.WARN('try fallback on connect/read upgrade response errors')
            if self.try_fallback(problem_stage='http connect&read'):
                return  # too early to die
        elif self.http_upgrade_error:
            err_code = self.http_upgrade_error
            self.http_upgrade_error = None
            if err_code != 404:
                self.WARN('try fallback on error={}'.format(message))
                if self.try_fallback(problem_stage='http upgrade'):
                    return  # too early to die

        if (details is not None) and self.error_ex_callback and self.error_ex_callback():
            self.error_ex_callback()(message, details=details)
            self.error_ex_callback = None
        elif self.error_callback and self.error_callback():
            self.error_callback()(message)
            self.error_callback = None
        if self.yaldi_proto:
            self.yaldi_proto.mark_as_failed()
        farewell_handshake = (code is None or code == EC_CLIENT_INACTIVITY_TIMEOUT
                              or code == EC_SERVER_INACTIVITY_TIMEOUT)
        self.close(farewell_handshake=farewell_handshake)

    def on_data(self, *args):
        if not self.is_closed():
            if self.callback and self.callback():
                self.callback()(*args)

    def add_chunk(self, data=None):
        if self._ignore_add or self._apphosted_asr:
            return

        if self.client_inactivity_timer:
            self.client_inactivity_timer.on_activity()

        # second part of abomination from line 204
        if self.is_closed() and not self.yet_to_connect:
            return

        if not self.last_chunk:
            if 'chunks_added' not in self.log_info:
                self.log_info['chunks_added'] = 0
            self.log_info['chunks_added'] += 1
            if data is None:
                self.DLOG('got last chunk')
                self.last_chunk = True
                data = b''
            else:
                self.DLOG('got new chunk size={}'.format(len(data)))
            req = AddData(audioData=data, lastChunk=self.last_chunk, appId=self.app_id, ip=self.client_ip)
            self.buffered.append(req)
            self.send_chunks()

    #
    #   VOICESERV-3235: dirty hack to fix core dumps
    #
    def force_last_chunk(self):
        self._ignore_add = True
        if not self._apphosted_asr:
            req = AddData(audioData=b'', lastChunk=self.last_chunk)
            self.send_protobuf(req, self.on_forced_last_chunk)

    def on_forced_last_chunk(self, send_result):
        pass

    def on_sent_chunk(self, future):
        if future.exception():
            result = False
        else:
            result = future.result()

        if not result or self.is_closed():
            self.on_error('AddData request failed')
            return

        if self.reading:
            return

        self.DLOG('waiting for result')
        self.reading = True
        if not self.yaldi_proto:
            return

        self.yaldi_proto.read_protobuf_ex(
            (AddDataResponse, InitResponse),
        ).add_done_callback(self.on_add_data_response)

    def send_chunks(self):
        if self._apphosted_asr:
            self.buffered = []
            return

        if self.is_closed():
            return

        if self.init_sent:
            while self.buffered:
                self.sent += 1
                data = self.buffered.pop(0)
                if self.sent == 1 and not data.audioData:
                    self.DLOG('last chunk and no data at all')
                    # skip sending chunk to backend here, so reset last_chunk for sending
                    # closeConnection=True in self.close()
                    self.last_chunk = False
                    self.on_data({
                        'durationProcessedAudio': '0',
                        'responseCode': 'OK',
                        'recognition': [],
                        'endOfUtt': True,
                        'bioResult': [],
                        'messagesCount': 1,
                    })
                    self.close()
                else:
                    self.send_protobuf(data, self.on_sent_chunk)

    def send_protobuf(self, data, on_sent_callback):
        if self.yaldi_proto:
            self.yaldi_proto.send_protobuf_ex(data).add_done_callback(on_sent_callback)
            self.time_last_send_protobuf = time.time()
        else:
            self.ERR('try send proto message to disconnected yaldi')

    def on_init_response(self, future):
        if future.exception():
            result = None
        else:
            result = future.result()
        self.read_init_response(result)

    def send_init_request(self):
        proto = None
        try:
            proto = proto_from_json(InitRequest, self.params)
        except Exception as exc:
            self.ERR('can not create InitRequest: {} from {}'.format(exc, self.params))
            self.EXC(exc)
            self.on_error('can not create InitRequest')
            return

        self.DLOG('sent init request')
        if self.client_inactivity_timer:
            self.client_inactivity_timer.on_activity()
            self.send_protobuf(proto, self.on_init_response)
        else:
            # self.close() already called, accurately close established connection
            proto.closeConnection = True
            self.send_protobuf(proto, self.on_send_close_connection)

    def on_send_close_connection(self, send_result):
        self.close()

    def try_fallback(self, problem_stage):
        if not self.fallback_topics:
            return False

        if self.yaldi_proto:
            self.yaldi_proto.close()
            self.yaldi_proto = None

        failed_host, failed_topic = self.yaldi_host, self.params['topic']
        self.yaldi_host, self.params['topic'] = self.fallback_topics.pop(0)
        self.WARN('{} (host={} topic={}) failed, switch to fallback (host={} topic={}) lang={}'.format(
            problem_stage,
            failed_host, failed_topic,
            self.yaldi_host, self.params['topic'],
            self.params['lang']
        ))
        self.mapped_topic = TOPIC_MAPS.get(self.params['topic'], self.params['lang'])
        self.connect()
        return True

    def process_init_response(self, res):
        if res.exception():
            self.process_init_response_impl(None)
        else:
            self.process_init_response_impl(res.result())

    def process_init_response_impl(self, res):
        if res is None or res.responseCode != 200:
            self.WARN('got error response for init request: ', res)
            if not self.try_fallback(problem_stage='init request'):
                self.on_error(
                    'init request (topic={}) failed: response={}'.format(self.params['topic'], res),
                    code=res.responseCode if res else 600
                )
        else:
            self.init_sent = True
            self.send_chunks()

    def read_init_response(self, result):
        if self.yaldi_inactivity_timer:
            self.yaldi_inactivity_timer.on_activity()

        if not result:
            if not self.try_fallback(problem_stage='connection/upgrade'):
                self.on_error('fail on init request sending')
        else:
            if self.yaldi_proto:
                self.yaldi_proto.read_protobuf_ex(InitResponse).add_done_callback(self.process_init_response)
            else:
                self.ERR('try send init proto message to disconnected yaldi')

    def on_add_data_response(self, future):
        if future.exception():
            if not self.is_closed():
                self.on_error('Exception during read_protobuf', code=600)
        else:
            self.on_add_data_response_impl(future.result())

    @gen.coroutine
    def on_add_data_response_impl(self, result):
        if self.yaldi_inactivity_timer:
            self.yaldi_inactivity_timer.on_activity()

        if self.invalid_null_response(result):
            return

        if self.invalid_response_type(result):
            return

        try:
            self.log_and_update_unistat_counters(result)

            first_eou = result.endOfUtt and not self.receive_end_of_utt
            if first_eou:
                self.INFO('recv first EOU')
            self.receive_end_of_utt |= result.endOfUtt
            self.time_last_recv_protobuf = time.time()
            self.processed += result.messagesCount

            data = proto_to_json(result)

            thrown_partial_fraction, data = self.process_partials_fraction(result, data)

            data = self.fixup_cache_key(result, data)

            is_trash, data = self.process_trash_result(data)

            core_debug, data = self.process_core_debug(data)

            self._yaldi_stat.process_new_data(data)

            if first_eou and self.options.wait_after_first_eou is not None:
                yield gen.sleep(self.options.wait_after_first_eou)

            if 'responses_got' not in self.log_info:
                self.log_info['responses_got'] = 0
            self.log_info['responses_got'] += 1

            self.process_asr_callback(data)

            if not self.process_error_code(result, data):
                return

            self.schedule_next_read(data)
        except Exception as exc:
            self.ERR('exception on asr callback: {}'.format(exc))
            self.EXC(exc)

    def invalid_null_response(self, result):
        if result:
            return False

        is_closed, reason = self.is_closed_2()
        if is_closed:
            return True  # if has already closed connection (from our side), handle result==None as Ok

        details = {}
        if not result:
            details['text'] = 'empty AddData response'
        else:
            details['text'] = 'YaldiStreamIsClosed: {}'.format(reason)

        details['scope'] = 'YaldiStream'
        self.log_info['last_chunk'] = self.last_chunk
        self.log_info['sent_chunks'] = self.sent
        details['log_info'] = self.log_info

        self.on_error('bad AddData response', code=600, details=details)
        return True

    def invalid_response_type(self, result):
        if isinstance(result, InitResponse):
            if not self.is_closed():
                self.on_error("bad AddData response: {}".format(result.message), code=result.responseCode)
            return True
        return False

    def log_and_update_unistat_counters(self, result):
        if result.responseCode != 200:
            self.log_asr_error(result)
            if not self.is_closed():
                self.increment_unistat_counter(result.responseCode)
        elif result.endOfUtt:
            self.increment_unistat_counter(200)

    def fixup_cache_key(self, message, result):
        if not message.HasField('cache_key'):
            result.pop('cacheKey', None)
        return result

    def process_partials_fraction(self, message, result):
        if message.HasField('thrown_partials_fraction'):
            return result.get('thrownPartialsFraction'), result
        else:
            result.pop('thrownPartialsFraction', None)
            return None, result

    def process_trash_result(self, result):
        is_trash = result.get('isTrash')
        if is_trash and self.options.ignore_trash_results:
            result['trash_recognition'] = result.pop('recognition', [])
            result['is_trash'] = True
            result['recognition'] = []
        return is_trash, result

    def process_core_debug(self, result):
        core_debug = result.get("coreDebug", None)

        if core_debug and self._system:
            payload = {
                "type": "AsrCoreDebug",
                "backend": self.unistat_counter,
                "ForEvent": self.message_id,
                "debug": core_debug,
                "FROM": "PYTHON",
            }

            self._system.logger.log_directive(
                payload,
                rt_log=self.rt_log,
            )

            self._system.write_directive(
                Directive(
                    'Uniproxy2',
                    'AnaLogInfo',
                    payload,
                    event_id=self.message_id
                ),
                log_message=False
            )

        return core_debug, result

    def process_asr_callback(self, result):
        self.on_data(result)

    def process_error_code(self, message, result):
        if message.responseCode != 200:
            self.on_error('closing on responseCode=%s' % (message.responseCode,))
            self.close(farewell_handshake=False)
            return False
        return True

    def schedule_next_read(self, result):
        if self.sent > self.processed:
            if self.yaldi_proto:
                self.yaldi_proto.read_protobuf_ex(
                    (AddDataResponse, InitResponse)
                ).add_done_callback(self.on_add_data_response)
                return True
        elif self.last_chunk:
            self.close(farewell_handshake=False)
        else:
            self.reading = False
        return False

    def increment_unistat_counter(self, code):
        if code == EC_CANCEL:
            self._yaldi_stat.finish()
        else:
            self._yaldi_stat.add_result_code(code)

        if self.has_finish_code:
            return

        self.has_finish_code = True
        GlobalCounter.increment_error_code(self.unistat_counter, code)
        try:
            if self._system:
                self._system.increment_stats(self.unistat_counter, code)
                if code != 200 and code != EC_CANCEL:
                    self.ERR('increment asr_error={}'.format(code))
        except ReferenceError:
            pass

    def log_asr_error(self, response):
        # VOICESERV-2017 debug WARN
        try:
            token = None
            if self.yaldi_proto:
                token = self.yaldi_proto.rt_log_token
            msg = 'got asr error={} rt_log_token={} eou={} '.format(
                response.responseCode,
                token,
                self.receive_end_of_utt,
            )
            if self.time_last_send_protobuf is not None:
                msg += ' last_send={}'.format(time.time() - self.time_last_send_protobuf)
            self.time_last_recv_protobuf = None
            if self.time_last_recv_protobuf is not None:
                msg += ' last_recv={}'.format(time.time() - self.time_last_recv_protobuf)
            self.WARN(msg)
        except Exception:
            self.EXC('fail VOICESERV-2017 WARN')

    def DLOG(self, *args):
        self._log.debug(self.session_id, *args, rt_log=self.rt_log)

    def INFO(self, *args):
        self._log.info(self.session_id, *args, rt_log=self.rt_log)

    def WARN(self, *args):
        self._log.warning(self.session_id, *args, rt_log=self.rt_log)

    def ERR(self, *args):
        self._log.error(self.session_id, *args, rt_log=self.rt_log)

    def EXC(self, exception):
        self._log.exception(exception, rt_log=self.rt_log)


class DoubleYaldiStream:
    def __init__(
        self,
        callback, error_callback, params, session_id, message_id,
        close_callback=None, host=None, port=None, unistat_counter='yaldi',
        rt_log=null_logger(), rt_log_label='asr', system=None, context_futures=(), error_ex_cb=None,
    ):
        self._log = Logger.get('.doubleyaldistream')
        self.params = params
        self.system = system
        self.last_asr_result = None
        self.session_id = session_id
        self.message_id = message_id
        self.eou_futures = None
        self.message_id = message_id
        self.results = [None, None]
        self.first_asr_duration_to_eou = None

        topics = params["topic"].split('+', 1)

        self.callback = weakref.WeakMethod(callback)
        self.error_callback = weakref.WeakMethod(error_callback)
        self.error_ex_callback = weakref.WeakMethod(error_ex_cb) if error_ex_cb else None
        if close_callback:
            self.close_callback = weakref.WeakMethod(close_callback)
        else:
            self.close_callback = None

        self.early_eou_time = time.monotonic()

        self.streams = []

        self.streams.append(YaldiStream(
            self.on_first_result_asr,
            self.on_first_error_asr,
            deepupdate(params, {
                "topic": topics[0],
                "advancedASROptions": {
                    "early_eou_message": False
                }
            }),
            session_id,
            message_id,
            close_callback=self.on_close_first_asr,
            unistat_counter='yaldi',
            rt_log=rt_log,
            rt_log_label='yaldi',
            system=system,
        ))

        self.streams.append(YaldiStream(
            self.on_second_result_asr,
            self.on_second_error_asr,
            deepupdate(params, {
                "topic": topics[1],
                "advancedASROptions": {
                    "early_eou_message": False,
                    "partial_results": conducting_experiment(
                        "vins_e2e_partials",
                        params,
                        uaas_flags=(system and system.uaas_flags)
                    ),
                },
                "disable_fallback": True
            }),
            session_id,
            message_id,
            close_callback=self.on_close_second_asr,
            unistat_counter='asr',
            rt_log=rt_log,
            rt_log_label='asr',
            system=system,
            context_futures=context_futures,
        ))

    def on_first_error_asr(self, err):
        if self.eou_futures:
            for x in self.eou_futures:
                if not x.done():
                    x.set_result(False)

        if self.error_callback and self.error_callback():
            self.error_callback()(err)

        self.on_close_asr()

    def on_second_error_asr(self, err):
        if self.eou_futures and len(self.eou_futures) > 1:
            self.eou_futures[1].set_result(False)

        self.results[1] = None
        self.on_close_asr('asr')

    def on_close_asr(self, asr_type=None):
        streams = []
        for stream in self.streams:
            if asr_type is None or stream.unistat_counter == asr_type:
                stream.close()
                continue
            streams.append(stream)
        self.streams = streams

    def on_close_first_asr(self):
        self.on_close_asr('yaldi')

    def on_close_second_asr(self):
        self.on_close_asr('asr')

    def is_first_result_ok(self, result):
        if not result:
            return False

        if conducting_experiment('e2e_merge_always_second', self.params):
            return False

        recognition = result.get("recognition", [])
        if recognition:
            for x in recognition[0].get("words", []):
                if re.match(r"[a-z]+", x.get("value", "").lower()):
                    return True
        return False

    @gen.coroutine
    def wait_and_merge_results(self):
        if self.eou_futures is not None:
            return

        self.early_eou_time = time.monotonic()

        self.eou_futures = [
            gen.Future(),
            gen.Future()
        ]

        eout_futures = [
            gen.with_timeout(
                datetime.timedelta(milliseconds=config['asr'].get('first_merge_timeout', 3000)),
                self.eou_futures[0]
            ),
            gen.with_timeout(
                datetime.timedelta(milliseconds=config['asr'].get('second_merge_timeout', 600)),
                self.eou_futures[1]
            )
        ]

        try:
            yield eout_futures
            self.eou_futures = []
        except (TimeoutError, gen.TimeoutError):
            if self.results[0] is None and self.results[1] is None:
                if self.error_callback and self.error_callback():
                    self.error_callback()("Timeout on merging asr results")
                return
        except Exception:
            if self.results[0] is None and self.results[1] is None:
                if self.error_callback and self.error_callback():
                    self.error_callback()("Error on merging asr results")
                return

        GlobalTimings.store(ASR_MERGE_HGRAM, time.monotonic() - self.early_eou_time)

        if self.callback and self.callback():
            result = self.results[1]
            if not result or self.is_first_result_ok(self.results[0]):
                result = self.results[0]
                GlobalCounter.ASR_MERGE_FIRST_SUMM.increment()
            else:
                GlobalCounter.ASR_MERGE_SECOND_SUMM.increment()
            self.callback()(result)
        else:
            self.WARN("No need in result at the end of merge", self.callback, self.callback())

        self.on_close_asr()
        if self.close_callback and self.close_callback():
            self.close_callback()()

    @gen.coroutine
    def on_first_result_asr(self, result):
        if result.get("earlyEndOfUtt", False):
            if self.first_asr_duration_to_eou is None:
                self.first_asr_duration_to_eou = result.get('durationProcessedAudio')
            self.wait_and_merge_results()  # << fork coroutine
            for x in self.streams:
                if x.unistat_counter == 'asr':
                    x.add_chunk()
            if self.callback and self.callback():
                self.callback()(result)
            return

        if result.get("endOfUtt", False):
            if self.first_asr_duration_to_eou is None:
                self.first_asr_duration_to_eou = result.get('durationProcessedAudio')
            self.wait_and_merge_results()  # << fork coroutine

            # VOICESERV-2247 dangerous piece of experimental code
            if (
                    conducting_experiment('e2e_merge_always_second', self.params)
                    and 'wait_after_first_eou' in self.params
            ):
                yield gen.sleep(float(self.params['wait_after_first_eou']))

            GlobalTimings.store(ASR_MERGE_FIRST_RESPONSE_HGRAM, time.monotonic() - self.early_eou_time)
            for x in self.streams:
                if x.unistat_counter == 'asr':
                    x.add_chunk()
            self.on_close_asr('yaldi')
            self.results[0] = result
            if self.eou_futures:
                self.eou_futures[0].set_result(True)
            else:
                self.WARN("No future to set first result")
        elif self.callback and self.callback() and not self.eou_futures:
            self.last_asr_result = copy.deepcopy(result)
            if not conducting_experiment("only_e2e_partials", self.params, self.system.uaas_flags):
                self.callback()(result)

    def on_second_result_asr(self, result):
        if result.get("endOfUtt", False):
            if self.first_asr_duration_to_eou is not None:
                offset_yaldi_eou = None
                try:
                    # NOTE: asr_duration is uint64,
                    # but convertor proto_to_json store it as string, so use cast & try/except
                    second_asr_duration_to_eou = result.get('durationProcessedAudio')
                    if second_asr_duration_to_eou is not None \
                            and second_asr_duration_to_eou >= self.first_asr_duration_to_eou:
                        offset_yaldi_eou = int(second_asr_duration_to_eou) - int(self.first_asr_duration_to_eou)
                        result['offsetYaldiEou'] = offset_yaldi_eou
                except Exception:
                    self.EXC('fail calculate offset EOU')
                if offset_yaldi_eou is not None:
                    GlobalTimings.store('offset_yaldi_eou', offset_yaldi_eou)

            self.wait_and_merge_results()  # << fork coroutine
            GlobalTimings.store(ASR_MERGE_SECOND_RESPONSE_HGRAM, time.monotonic() - self.early_eou_time)
            _ensure_normalized_recognition(result.get("recognition", []))
            self.results[1] = result
            if self.eou_futures:
                self.eou_futures[1].set_result(True)
            else:
                self.WARN("No future to set second result")
            self.on_close_asr('asr')
        elif self.callback and self.callback() and result.get("recognition") and not self.eou_futures:
            if conducting_experiment("only_e2e_partials", self.params, self.system.uaas_flags):
                result["e2e_recognition"] = result["recognition"]
                self.callback()(result)
            elif self.last_asr_result:
                result["e2e_recognition"] = result["recognition"]
                result["recognition"] = self.last_asr_result["recognition"]
                result["messagesCount"] = 0
                self.callback()(result)

    def add_chunk(self, data=None):
        for stream in self.streams:
            stream.add_chunk(data)

    def is_closed(self):
        for stream in self.streams:
            if not stream.is_closed():
                return False
        return True

    def close(self):
        if not self.is_closed():
            self.on_close_asr()

    def WARN(self, *args):
        self._log.warning(self.session_id, self.message_id, *args)

    def EXC(self, *args):
        self._log.exception(self.session_id, self.message_id, *args)


def get_yaldi_stream_type(topic):
    if '+' in topic:
        return DoubleYaldiStream
    if topic.startswith(('galdi', 'google')):
        return GaldiStream
    return YaldiStream
