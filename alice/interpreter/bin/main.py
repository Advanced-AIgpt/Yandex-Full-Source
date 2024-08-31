import logging
import json

import click
import gunicorn.app.base

from flask import Flask


class GunicornApp(gunicorn.app.base.BaseApplication):

    def __init__(self,
                 app: Flask,
                 port: int,
                 workers: int,
                 threads: int,
                 timeout: int,
                 conf: str,
                 ):
        self._app = app
        self._app.raw_config = json.load(open(conf))
        self._app.config.from_mapping(self._app.raw_config)

        self._port = port
        self._workers = workers
        self._threads = threads
        self._timeout = timeout

        super(GunicornApp, self).__init__()

    def load_config(self):
        self.cfg.set('bind', ['[::]:%s' % self._port])
        self.cfg.set('workers', self._workers)
        self.cfg.set('threads', self._threads)
        self.cfg.set('timeout', self._timeout)
        self.cfg.set('backlog', self._workers)
        self.cfg.set('worker_class', 'gevent')
        self.cfg.set('accesslog', '-')  # stdout

    def load(self):
        return self._app


@click.command()
@click.option('-p', '--port', type=int, required=True, help='Server port')
@click.option('-w', '--workers', default=1, help='Workers count', show_default=True)
@click.option('-t', '--timeout', default=10, help='Worker inactivity timeout', show_default=True)
@click.option('-l', '--logging-level', default='DEBUG', type=click.Choice(['DEBUG', 'INFO', 'WARNING', 'ERROR']))
@click.option('--threads', default=8, help='Threads per worker', show_default=True)
@click.argument('config', type=click.Path(exists=True))
def main(port: int, workers: int, threads: int, timeout: int, logging_level: str, config: str):
    logging.basicConfig(level=logging_level)

    # The gevent server adapter needs to patch some modules before they are imported
    from gevent import monkey
    monkey.patch_all()

    from alice.interpreter.lib.app import app
    GunicornApp(app, port, workers, threads, timeout, config).run()


if __name__ == '__main__':
    main()
