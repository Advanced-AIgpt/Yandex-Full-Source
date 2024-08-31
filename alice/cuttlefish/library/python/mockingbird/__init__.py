from .server import MockServer, HttpBackend, AppHostGrpcBackend
from contextlib import asynccontextmanager, contextmanager


class Mockingbird:
    def __init__(self):
        self._backends = {}
        self._srv = None

    @asynccontextmanager
    async def run(self):
        if self._srv is not None:
            raise RuntimeError("already running")

        async with MockServer(self._backends.values()) as srv:
            try:
                self._srv = srv
                yield self
            finally:
                self._srv = None

    def endpoint_for(self, name):
        """Returns endpoint (host, port) for a given backend"""
        return self._srv.endpoint_for(name)

    def backend(self, name):
        """Returns backend of a given name"""
        return self._backends[name]

    @property
    def backends(self):
        return self._backends.values()

    @property
    def servants(self):
        """Only for running instances"""
        return self._srv.servants

    def add_http_backend(self, name, port, callback=None):
        if name in self._backends:
            raise RuntimeError(f"mock for HTTP '{name}' already exists")
        self._backends[name] = HttpBackend(name=name, callback=callback, port=port)

    def add_apphost_backend(self, name, paths, port, callback=None):
        if name in self._backends:
            raise RuntimeError(f"mock for AppHost-gRPC '{name}' already exists")
        self._backends[name] = AppHostGrpcBackend(name=name, paths=paths, callback=callback, port=port)

    @contextmanager
    def override(self, callback_patch):
        """Temporary changes callbacks for given backends"""
        prev = {}
        for name, callback in callback_patch.items():
            backend = self.backend(name)
            prev[name] = backend.callback
            backend.callback = callback

        try:
            yield self
        finally:
            for name, callback in prev.items():
                self.backend(name).callback = callback

    @property
    def records(self):
        res = {}
        for name, backend in self._backends.items():
            if backend._history:
                res[name] = [i for i in backend._history]
        return res

    def clear_records(self):
        for backend in self.backends:
            backend._history.clear()
