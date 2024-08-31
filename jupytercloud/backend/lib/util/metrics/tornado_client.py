import datetime
import traceback
import weakref

from tornado import curl_httpclient, httpclient, simple_httpclient


PROCESSED = 'processed_requests_rate'
TIMEOUTED = 'timeouted_requests_rate'
INSTANTIATED = 'instantiated_count'
DISPOSED = 'disposed_count'
SENSOR_PREFIX = 'async_http_client.'


class HTTPClientInfo:
    def __init__(self, client_name, options, client, metric_registry):
        self.name = client_name
        self.options = options
        self.creation_time = datetime.datetime.now()

        self.counters = {}

        for metric_name in (INSTANTIATED, DISPOSED):
            labels = self._get_labels(metric_name)
            self.counters[metric_name] = metric_registry.counter(labels)

        for metric_name in (PROCESSED, TIMEOUTED):
            labels = self._get_labels(metric_name)
            self.counters[metric_name] = metric_registry.rate(labels)

        self.traceback = self._get_traceback()

        self._ref = weakref.ref(client, self.dispose)

    def _get_labels(self, metric_name):
        return {
            'client_name': self.name,
            'sensor': SENSOR_PREFIX + metric_name,
        }

    def update_client(self, client):
        self._ref = weakref.ref(client, self.dispose)

    def dispose(self, _):
        self.counters[DISPOSED].inc()

    @staticmethod
    def _get_traceback():
        tb = traceback.extract_stack()

        # 8 первых фрейма - это всякий бутстрап-код, типа
        # python3/src/Lib/runpy.py:194 _run_module_as_main,
        # jupyterhub/app.py:2733 launch_instance -> loop.start()
        # и asyncio-кода запуска
        # Три последних фрейма - это AsyncHTTPClient.__new__,
        # данный __init__ и данная функция
        short = tb[8:-3]

        # Чистим декораторы, устанавливающие контекст логирования
        filtered = [
            frame for frame in short
            if not (
                frame.filename == 'jupytercloud/backend/util/logging.py' and
                frame.name == '_method'
            )
        ]

        return filtered

    @property
    def client(self):
        return self._ref()

    @property
    def active_requests(self):
        if isinstance(self.client, simple_httpclient.SimpleAsyncHTTPClient):
            return len(self.client.active)
        elif isinstance(self.client, curl_httpclient.CurlAsyncHTTPClient):
            return len(self.client._curls) - len(self.client._free_list)
        else:
            return 0

    @property
    def queued_requests(self):
        if isinstance(self.client, simple_httpclient.SimpleAsyncHTTPClient):
            return len(self.client.queue)
        elif isinstance(self.client, curl_httpclient.CurlAsyncHTTPClient):
            return len(self.client._requests)
        else:
            return 0

    @property
    def is_alive(self):
        return bool(self.client)

    @property
    def pretty_traceback(self):
        return '\n'.join(
            f'{f.filename}:{f.lineno} {f.name} -> {f.line}'
            for f in self.traceback
        )

    def push_solomon_metrics(self, registry):
        for metric_name, value in (
            ('active_requests', self.active_requests),
            ('queued_requests', self.queued_requests),
        ):
            labels = self._get_labels(metric_name)
            registry.int_gauge(labels).set(value)


class ClientPatcher:
    def _dispose_http_client(self, client_name):
        self._http_clients.pop(client_name)

    @staticmethod
    def patch_method_counter(cls, method_name, counter_name):
        old_method = getattr(cls, method_name)

        def new_method(self, *args, **kwargs):
            result = old_method(self, *args, **kwargs)

            self._http_client_info.counters[counter_name].inc()

            return result

        setattr(cls, method_name, new_method)

    @classmethod
    def patch(klass, parent, registry):
        parent._http_clients = {}

        old_new = httpclient.AsyncHTTPClient.__new__

        # ensure we patch only once
        assert old_new.__module__ == 'tornado.httpclient'

        def __new__(cls, force_instance=False, **kwargs):
            client_name = kwargs.pop('client_name', 'default')

            assert not force_instance, \
                'patched client does not support force_instance, use client_name instead'

            if client_name not in parent._http_clients:
                instance = old_new(cls, force_instance=True, **kwargs)
                options = kwargs.copy()

                client_info = HTTPClientInfo(client_name, options, instance, registry)
                instance._http_client_info = client_info

                parent._http_clients[client_name] = client_info

            client_info = parent._http_clients[client_name]
            client_info.counters[INSTANTIATED].inc()

            if not client_info.client:
                instance = old_new(cls, force_instance=True, **kwargs)
                instance._http_client_info = client_info
                client_info.update_client(instance)

            return client_info.client

        httpclient.AsyncHTTPClient.__new__ = __new__
        klass.patch_method_counter(
            curl_httpclient.CurlAsyncHTTPClient,
            '_handle_timeout',
            TIMEOUTED,
        )


class CleanArgsPatcher:
    """Removes non-standard arguments from AsyncHTTPClient calls"""

    @staticmethod
    def patch():
        old_new = httpclient.AsyncHTTPClient.__new__

        def __new__(cls, **kwargs):
            if 'client_name' in kwargs:
                del kwargs['client_name']
            return old_new(cls, **kwargs)

        httpclient.AsyncHTTPClient.__new__ = __new__
