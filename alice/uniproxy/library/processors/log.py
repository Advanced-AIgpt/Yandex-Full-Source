from . import EventProcessor, register_event_processor

from alice.uniproxy.library.events import Directive

from alice.uniproxy.library.global_counter import GlobalTimings
from alice.uniproxy.library.global_counter import GlobalCounter
from alice.uniproxy.library.global_counter.uniproxy import CLIENT_VINS_RESPONSE_HGRAM
from alice.uniproxy.library.global_counter.uniproxy import CLIENT_EOU_HGRAM
from alice.uniproxy.library.global_counter.uniproxy import CLIENT_FIRST_MERGED_HGRAM
from alice.uniproxy.library.global_counter.uniproxy import CLIENT_FIRST_PARTIAL_HGRAM
from alice.uniproxy.library.global_counter.uniproxy import CLIENT_FIRST_SYNTH_HGRAM
from alice.uniproxy.library.global_counter.uniproxy import CLIENT_LAST_PARTIAL_HGRAM
from alice.uniproxy.library.global_counter.uniproxy import CLIENT_LAST_SYNTH_HGRAM


@register_event_processor
class Spotter(EventProcessor):
    def process_event(self, event):
        super(Spotter, self).process_event(event)
        if event.payload.get('vinsMessageId', False):
            self.system.log_spotter_vins_message_id = event.payload.get('vinsMessageId')

    def process_streamcontrol(self, _):
        self.system.write_directive(Directive("Log", "Ack", {}, self.event.message_id))
        self.close()
        return EventProcessor.StreamControlAction.Close


class AudioStream(Spotter):
    pass


@register_event_processor
class RequestStat(EventProcessor):
    def process_event(self, event):
        def store_interval(name, end, begin):
            if begin is None or end is None:
                return
            GlobalTimings.store(name, (float(end) - float(begin)) * 0.001)

        super().process_event(event)
        spotter = event.payload.get("spottingRejectedError")
        if spotter is not None:
            if spotter:
                GlobalCounter.SPOTTER_ERR_SUMM.increment()
            else:
                GlobalCounter.SPOTTER_OK_SUMM.increment()
        timestamps = event.payload.get("timestamps")
        if timestamps:
            on_recognition_end_time = timestamps.get("onRecognitionEndTime")

            on_first_merged_time = timestamps.get("onFirstMessageMergedTime")
            on_first_partial_time = timestamps.get("onFirstNonEmptyPartialTime")
            on_first_synth_chunk_time = timestamps.get("onFirstSynthesisChunkTime")
            on_last_partial_time = timestamps.get("onLastCompletedPartialTime")
            on_last_synth_chunk_time = timestamps.get("onLastSynthesisChunkTime")
            on_vins_response_time = timestamps.get("onVinsResponseTime")

            suffix = ""
            if self.system.is_quasar:
                suffix = "_quasar"

            try:
                store_interval(CLIENT_FIRST_MERGED_HGRAM + suffix, on_first_merged_time, 0.0)
                store_interval(CLIENT_FIRST_PARTIAL_HGRAM + suffix, on_first_partial_time, on_first_merged_time)
                store_interval(CLIENT_LAST_PARTIAL_HGRAM + suffix, on_last_partial_time, on_first_partial_time)
                store_interval(CLIENT_EOU_HGRAM + suffix, on_recognition_end_time, on_last_partial_time)
                store_interval(CLIENT_VINS_RESPONSE_HGRAM + suffix, on_vins_response_time, on_recognition_end_time)
                store_interval(CLIENT_FIRST_SYNTH_HGRAM + suffix, on_first_synth_chunk_time, on_vins_response_time)
                store_interval(CLIENT_LAST_SYNTH_HGRAM + suffix, on_last_synth_chunk_time, on_first_synth_chunk_time)
            except Exception:
                pass

        # VOICESERV-3340 Ack for RequestStat
        #   assuming `ack` field in RequestState message is int64_t
        #   future versions of uniproxy may reply with any positive int64_t value if type of the original field is not int
        ack_required = event.payload.get('ack', 0)
        if ack_required:
            self.system.write_directive(Directive('Log', 'RequestStatAck', {'ack': ack_required}, self.event.message_id))

        self.close()
