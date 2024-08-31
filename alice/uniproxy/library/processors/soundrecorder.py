from . import EventProcessor, register_event_processor


@register_event_processor
class ModeChanged(EventProcessor):
    def process_streamcontrol(self, _):
        self.close()


@register_event_processor
class DirectionChanged(EventProcessor):
    def process_streamcontrol(self, _):
        self.close()
