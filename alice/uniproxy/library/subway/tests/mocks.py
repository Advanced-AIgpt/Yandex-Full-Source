import logging
import time
import uuid
import urllib

import tornado.concurrent

from yatest.common import execute
from yatest.common import binary_path
from yatest.common import network

from alice.uniproxy.library.subway.pull_client import PullClient, ClientType


def with_subway(fn):
    def wrapper(*args, **kwargs):
        flog = logging.getLogger('subway.mixin')

        try:
            with network.PortManager() as pm:
                port = pm.get_port(7777)
                flog.info('port assigned %d', port)

                app = execute([
                    binary_path('alice/uniproxy/bin/uniproxy-subway/uniproxy-subway'),
                    '--port', str(port),
                    '--verbose'
                ], wait=False)

                starting_limit = 5  # give max 5 sec for starting subway
                deadline = time.time() + starting_limit
                start_error = 'yet not started'
                while time.time() < deadline:
                    try:
                        resp = urllib.request.urlopen(f'http://localhost:{port}/ping')
                        if resp.status == 200:
                            start_error = None
                            break
                        else:
                            start_error = f'recv on /ping return code={resp.status}'
                    except Exception as exc:
                        start_error = str(exc)

                if start_error is not None:
                    raise Exception(f'with_subway: fail start: {start_error}')
                flog.info('with_subway: app started')

                client = PullClient(port, nocache=True, wait=True)
                client.start()

                flog.info('with_subway: client created')

                kwargs.update({'client': client})
                fn(*args, **kwargs)

                flog.info('with_subway: test called')

                app.kill()

                flog.info('with_subway: app killed')

                client = None
        except Exception as ex:
            flog.exception(ex)
            raise

    return wrapper


class UnisystemMock:
    def __init__(self, guid):
        self.subway_uid = guid
        self.session_id = str(uuid.uuid4())
        self.message = tornado.concurrent.Future()
        self.subway_client_type = ClientType.GUID
        self.device_model = None
        self.session_data = {}

    def on_subway_message(self, data):
        self.message.set_result(data)
