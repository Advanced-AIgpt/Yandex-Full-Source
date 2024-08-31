import alice.apphost.fixture as apphost
import alice.bass.fixture as bass
import alice.hollywood.fixture as hollywood
import alice.megamind.fixture as megamind
import alice.kronstadt.fixture as kronstadt


class _DummyService:
    def __init__(self, port, dummy=False, *args, **kwargs):
        self._dummy = dummy
        super().__init__(port, *args, **kwargs)

    def __enter__(self):
        if self._dummy:
            return self
        return super().__enter__()

    def __exit__(self, type, value, traceback):
        if self._dummy:
            return
        super().__exit__(type, value, traceback)

    def __bool__(self):
        return not self._dummy

    def wait_port(self):
        if not self._dummy:
            super().wait_port()


class AppHost(_DummyService, apphost.AppHost):
    pass


class Bass(_DummyService, bass.Bass):
    pass


class Hollywood(_DummyService, hollywood.Hollywood):
    pass


class Megamind(_DummyService, megamind.Megamind):
    pass


class Kronstadt(_DummyService, kronstadt.Kronstadt):
    pass
