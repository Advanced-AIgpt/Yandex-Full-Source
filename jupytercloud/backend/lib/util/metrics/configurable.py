from jupytercloud.backend.lib.util.logging import log_request
from library.python.monlib.encoder import dumps as solomon_dumps
from library.python.monlib.metric_registry import HistogramType, MetricRegistry

from .tornado_client import ClientPatcher


def make_solomon_request_logger(registry, histogram_cache, old_log_request):
    # Solomon stuff:
    # MANDATORY:
    # project = "jupyter-cloud"
    # cluster = "production"/"testing"
    # service = "hub"/"salt"/"idm" ... etc
    # sensor  = actual name of the  metric

    def register_method_rate(handler_name, method, code):
        labels = dict(
            sensor='handler.request_rate',
            handler=handler_name,
            method=method,
            code=code,
        )
        rate = registry.rate(labels)
        rate.inc()

    def register_method_duration(handler_name, request_time):
        labels = dict(
            sensor='handler.request_duration_ms',
            handler=handler_name,
        )
        labels_key = frozenset(labels.items())

        hist = histogram_cache.get(labels_key)
        if hist is None:
            hist = registry.histogram_rate(
                labels,
                HistogramType.Explicit,
                buckets=[5, 10, 15, 25, 50, 100, 250, 500, 1000, 3500, 10000],
            )
            histogram_cache[labels_key] = hist

        hist.collect(request_time * 1000)  # seconds -> ms

    def new_log_request(handler):
        old_log_request(handler)

        handler_name = f'{handler.__class__.__module__}.{type(handler).__name__}'
        method = handler.request.method
        status = str(handler.get_status())

        register_method_rate(handler_name, method, status)
        register_method_duration(handler_name, handler.request.request_time())

    return new_log_request


class MetricsConfigurableMixin:
    _http_clients = None
    _solomon_registry = None
    _solomon_histogram_cache = None

    def __init__(self, *args, **kwargs):
        self._solomon_registry = MetricRegistry()
        # separate cache allows for easier overrides of default values
        self._solomon_histogram_cache = {}

        ClientPatcher.patch(self, self._solomon_registry)

        super().__init__(*args, **kwargs)

    def set_jupyter_cloud_request_logger(self):
        # there is no way to make it `init_tornado_settings` due to
        # multiple inheritance hell

        self.tornado_settings['log_function'] = make_solomon_request_logger(
            self._solomon_registry,
            self._solomon_histogram_cache,
            log_request,
        )

    @property
    def http_clients_info(self):
        return self._http_clients

    def push_solomon_metrics(self):
        for client_info in self.http_clients_info.values():
            client_info.push_solomon_metrics(self._solomon_registry)

    def solomon_dumps(self):
        self.push_solomon_metrics()
        return solomon_dumps(self._solomon_registry, format='json')
