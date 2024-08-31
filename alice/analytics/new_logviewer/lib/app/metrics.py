import attr
import flask

from library.python.monlib.metric_registry import (
    MetricRegistry,
    HistogramType
)
from library.python.monlib.encoder import dumps

CONTENT_TYPE_SPACK = "application/x-solomon-spack"
CONTENT_TYPE_JSON = "application/json"


@attr.s
class LogviewerMetricsConfig:
    search_solomon_buckets: list[int] = attr.ib()
    skill_solomon_buckets: list[int] = attr.ib()


class LogviewerMetrics:
    def __init__(self):
        self._registry = None
        self.search_response_time = None
        self.search_requests_per_second = None
        self.search_errors_per_second = None
        self.skill_response_time = None

    def init_from_config(self, config: LogviewerMetricsConfig):
        self._registry = MetricRegistry()

        self.search_response_time = self._registry.histogram_rate(
            labels={"sensor": "search.response_time"},
            hist_type=HistogramType.Explicit,
            buckets=config.search_solomon_buckets
        )
        self.search_requests_per_second = self._registry.rate(
            {"sensor": "search.requests_per_second"}
        )
        self.search_errors_per_second = self._registry.rate(
            {"sensor": "search.errors_per_second"}
        )

        self.skill_response_time = self._registry.histogram_rate(
            labels={"sensor": "skill.response_time"},
            hist_type=HistogramType.Explicit,
            buckets=config.skill_solomon_buckets
        )

    def to_response(self) -> flask.Response:
        if flask.request.headers["accept"] == CONTENT_TYPE_SPACK:
            return flask.Response(dumps(self._registry), mimetype=CONTENT_TYPE_SPACK)
        return flask.Response(dumps(self._registry, format="json"), mimetype=CONTENT_TYPE_JSON)
