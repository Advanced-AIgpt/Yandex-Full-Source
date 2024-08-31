from alice.uniproxy.library.musicstream import MusicStream2
from alice.uniproxy.library.global_counter import GlobalCounter


GlobalCounter.init()


class MusicStreamClient:
    def __init__(self, params={}):
        self.music_stream = MusicStream2(
            self.on_music_result,
            self.on_music_error,
            params
        )

    def on_music_result(self, result, finish=True):
        pass

    def on_music_error(self, err):
        pass


def test_create_music_stream():
    client = MusicStreamClient()
    assert client.music_stream
