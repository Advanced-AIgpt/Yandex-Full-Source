from . import EventProcessor, register_event_processor
from alice.uniproxy.library.extlog import AccessLogger
from alice.uniproxy.library.musicstream import MusicStream2
from alice.uniproxy.library.backends_asr import YaldiStream, get_yaldi_stream_type
from alice.uniproxy.library.events import Directive
from alice.uniproxy.library.utils.experiments import conducting_experiment

import copy


def _is_normalized_not_empty(rec):
    return rec.get("normalized", "").strip() != ""


def _is_words_not_empty(rec):
    return ''.join([word.get('value', '') for word in rec.get("words", [])]).strip() != ""


def _prepare_asr_result(result, in_place=False, is_robot=False):
    """ Prepare ASR.Result for sending: drop all not first hypothesis with empty "normalized".
        Return corrected asr_result.
    """

    recognition = result.get("recognition")

    if (recognition is None) or (not result.get("endOfUtt", False)):
        return result

    prefix_to_keep = 1

    if len(recognition) > prefix_to_keep:
        if is_robot:
            good_hypothesis = list(filter(_is_words_not_empty, recognition[prefix_to_keep:]))
        else:
            good_hypothesis = list(filter(_is_normalized_not_empty, recognition[prefix_to_keep:]))

        if len(good_hypothesis) + prefix_to_keep < len(recognition):
            # make copy only if needed
            if in_place:
                result["recognition"][prefix_to_keep:] = good_hypothesis
                return result

            else:
                return_value = copy.deepcopy(result)
                return_value["recognition"][prefix_to_keep:] = good_hypothesis
                return return_value

    # return unchanged structure
    return result


@register_event_processor
class Recognize(EventProcessor):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.asr_stream_type = YaldiStream
        self.first_logger = AccessLogger("asr_first_chunk", self.system, rt_log=self.rt_log)
        self.full_logger = AccessLogger("asr_full", self.system, rt_log=self.rt_log)
        self.data_size = 0
        self.event = None
        self.streams = []

    def close(self):
        for stream in self.streams:
            stream.close()
        self.streams = []

        super(Recognize, self).close()

    def close_stream(self, stream_type):
        streams = []

        for stream in self.streams:
            if isinstance(stream, stream_type):
                stream.close()
                continue
            streams.append(stream)
        self.streams = streams

    def process_streamcontrol(self, _):
        for stream in self.streams:
            stream.add_chunk()
        return EventProcessor.StreamControlAction.Close

    def add_data(self, data):
        self.data_size += len(data)

        if self.first_logger:
            self.first_logger.end(code=200, size=self.data_size)
            self.first_logger = None

        for stream in self.streams:
            stream.add_chunk(data)

    def on_result_asr(self, result):
        if result.get("earlyEndOfUtt", False):
            self.DLOG("ASR got earlyEndOfUtt (ignore message)")
            return

        if self.full_logger and result.get("endOfUtt", False):
            self.full_logger.end(code=200, size=self.data_size)
            self.full_logger = None

        if not self.is_closed():
            if conducting_experiment('enable_continue_streaming', self.payload_with_session_data):
                result["continue_streaming"] = True
            if result.get('endOfUtt', False):
                self.INFO("asr_proc send to client Result.eou=true")
            Recognize.write_directive(self, "Result", result)
            if "continue_streaming" in result:
                result.pop("continue_streaming")

    def on_error_asr(self, err):
        if self.full_logger:
            self.full_logger.end(code=500, size=self.data_size)
            self.full_logger = None
        self.dispatch_error(err, close=(not self.streams))

    def on_error_asr_ex(self, message, details=None):
        if self.full_logger:
            self.full_logger.end(code=500, size=self.data_size)
            self.full_logger = None
        self.dispatch_error_ex(message, details=details, close=(not self.streams))

    def on_close_asr(self):
        self.close_stream(self.asr_stream_type)
        if not self.streams:
            self.close()

    def on_result_music(self, result, finish=True):
        if self.is_closed():
            return

        self.write_directive("MusicResult", result)
        if finish:
            self.close_stream(MusicStream2)
        if not self.streams:
            self.close()

    def on_error_music(self, err):
        if self.is_closed():
            return

        self.close_stream(MusicStream2)
        self.dispatch_error(err, close=(not self.streams))

    def write_directive(self, name, result):
        if name == "Result":
            # do not remove empty normalized from source structure for quasar
            is_robot = False
            if conducting_experiment('enable_multiple_hypotheses_to_client', self.payload_with_session_data):
                is_robot = self.system.is_robot

            if not conducting_experiment('KEEP_CORE_DEBUG', self.payload_with_session_data):
                result.pop('coreDebug', None)
            result = _prepare_asr_result(result, in_place=not self.system.is_quasar, is_robot=is_robot)

        try:
            self.system.write_directive(Directive("ASR", name, result, self.event.message_id))
        except ReferenceError:
            pass

    def process_event(self, event):
        super(Recognize, self).process_event(event)

        topic = self.payload_with_session_data.get('topic', 'dialogeneral')

        self.first_logger.start(event_id=self.event.message_id, resource=topic)
        self.full_logger.start(event_id=self.event.message_id, resource=topic)

        self.asr_stream_type = get_yaldi_stream_type(topic)

        unistat_counter = 'asr' if topic.endswith('gpu') else 'yaldi'
        rtlog_label = unistat_counter

        if 'music_request2' in self.payload_with_session_data:
            self.streams.append(MusicStream2(
                self.on_result_music,
                self.on_error_music,
                self.payload_with_session_data,
                uuid=self.system.session_data.get('uuid'),
                api_key=self.system.session_data.get('apiKey'),
                oauth_token=self.system.get_oauth_token(),
                client_ip=self.system.client_ip,
                client_port=self.system.client_port,
                rt_log=self.rt_log,
            ))
            if self.payload_with_session_data.get('recognize_music_only', False):
                return

        self.streams.append(self.asr_stream_type(
            self.on_result_asr,
            self.on_error_asr,
            self.payload_with_session_data,
            self.system.session_id,
            self.event.message_id,
            unistat_counter=unistat_counter,
            close_callback=self.on_close_asr,
            host=self.system.srcrwr['ASR'],
            port=self.system.srcrwr.ports.get('ASR'),
            rt_log=self.rt_log,
            rt_log_label=rtlog_label,
            system=self.system,
            error_ex_cb=self.on_error_asr_ex,
        ))
