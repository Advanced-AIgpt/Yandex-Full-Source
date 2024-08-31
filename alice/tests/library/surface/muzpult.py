from alice.tests.library.uniclient import event


class Muzpult(object):
    def __init__(self, alice):
        self._alice = alice
        self._pause = False

    def pause(self):
        self._pause = True

    def unpause(self):
        self._pause = False

    def play(self, id, type, start_from_track_id=None, offset_sec=None):
        return self._alice._send_event(event.MusicPlay(
            object_id=id,
            object_type=type,
            start_from_track_id=start_from_track_id,
            offset_sec=offset_sec,
        ))

    def play_next(self):
        return self._alice._send_event(event.PlayerNextTrack(
            set_pause=self._pause,
        ))

    def play_prev(self):
        return self._alice._send_event(event.PlayerPrevTrack(
            set_pause=self._pause,
        ))
