class FakeYaldiStream:
    STREAMS = []

    def __init__(self, callback, *args, **kwargs):
        self.callback = callback
        FakeYaldiStream.STREAMS.append(self)

    def add_chunk(self, data=None):
        pass

    def close(self, *args, **kwargs):
        pass


def fake_get_yaldi_stream_type(topic):
    return FakeYaldiStream
