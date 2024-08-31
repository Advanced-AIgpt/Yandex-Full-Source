import time

import tornado.ioloop

from threading import Thread


class TornadoIOLoop:
    def __enter__(self):
        self._thread = Thread(target=lambda: tornado.ioloop.IOLoop.current().start())
        self._thread.start()
        time.sleep(1)

        return self

    def __exit__(self, *args, **kwargs):
        tornado.ioloop.IOLoop.instance().stop()
        self._thread.join()
