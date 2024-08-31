import datetime
from tornado import gen
from tornado.concurrent import Future
from tornado.ioloop import IOLoop

from . import EventProcessor, register_event_processor

from alice.uniproxy.library.events import Directive
from alice.uniproxy.library.global_counter import GlobalCounter


@register_event_processor
class Generate(EventProcessor):
    def __init__(self, *args, **kwargs):
        super(Generate, self).__init__(*args, **kwargs)
        self.on_close = None
        self.backward_apphosted_tts_rtlog_token = None
        self.finish_future = Future()

    def set_finish_future(self):
        self.finish_future.set_result(None)

    def close(self):
        if self.on_close:
            self.on_close()
            self.on_close = None

        if self.set_finish_future:
            self.set_finish_future()
            self.set_finish_future = None

        super(Generate, self).close()

    def process_event(self, event):
        super(Generate, self).process_event(event)

        GlobalCounter.U2_COUNT_BACKWARD_APPHOSTED_TTS_SUMM.increment()
        self._send_backward_generate()
        self._schedule_deadline_timer()

    def _send_backward_generate(self):
        # Just for more clear setrace
        # All apphost rtlogs will be under uniproxy2 not under python uniproxy
        self.backward_apphosted_tts_rtlog_token = self.rt_log.log_child_activation_started('backward_apphosted_tts')
        directive = Directive(
            'Uniproxy2',
            'Generate',
            self.event.payload,
            self.event.message_id,
        )
        self.system.write_directive(directive, self)

    def _schedule_deadline_timer(self):
        IOLoop.current().spawn_callback(self._deadline_timer_callback)

    def _get_timeout_seconds(self):
        return self.payload_with_session_data.get('settings_from_manager', {}).get('mvp_apphosted_tts_generate_backward_request_timeout_seconds', 30)

    @gen.coroutine
    def _deadline_timer_callback(self):
        try:
            yield gen.with_timeout(datetime.timedelta(seconds=self._get_timeout_seconds()), self.finish_future)

            self.rt_log.log_child_activation_finished(self.backward_apphosted_tts_rtlog_token, True)
            self.system.logger.log_directive({
                "ForEvent": self.event.message_id,
                "type": "BackwardAppHostedTtsSuccess",
                "Body": {"reason": "AppHostedTts success!"},
            })
        except Exception as exc:
            GlobalCounter.U2_FAILED_BACKWARD_APPHOSTED_TTS_SUMM.increment()

            error = str(exc)

            self.rt_log.log_child_activation_finished(self.backward_apphosted_tts_rtlog_token, False)
            self.system.logger.log_directive({
                "ForEvent": self.event.message_id,
                "type": "BackwardAppHostedTtsFailed",
                "Body": {"reason": "AppHostedTts error: " + error},
            })

            if not self.is_closed():
                self.dispatch_error(error)
                self.close()
