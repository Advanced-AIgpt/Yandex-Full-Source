from enum import Enum, unique


class FakeYabioStream:
    STREAMS = []

    @unique
    class YabioStreamType(Enum):
        Score = "yabio-score"
        Classify = "yabio-classify"

    def __init__(self, stream_type, *args, **kwargs):
        self.stream_type = stream_type
        self._closed = False
        self.last_chunk = False
        FakeYabioStream.STREAMS.append(self)

    def add_chunk(self, data=None, need_result=False, last_spotter_chunk=False, last_chunk=False, text=None):
        return True

    def last_result_is_actual(self):
        return False

    def close(self, *args, **kwargs):
        self._closed = True

    @staticmethod
    def get_group_id(params):
        return None
