import tornado.gen


class FakeSpotterStream:
    STREAMS = []

    def __init__(self, callback, *args, smart_activation=None, **kwargs):
        self.callback = callback
        FakeSpotterStream.STREAMS.append(self)

    def add_chunk(self, data=None):
        pass

    def close(self, *args, **kwargs):
        pass

    def get_back(self):
        return self

    @tornado.gen.coroutine
    def final(self, _):
        pass

    @tornado.gen.coroutine
    def fast_circuit(self):
        return True
