# -*- coding: utf8 -*-
# flake8: noqa
"""
Uxie graphics
"""
import inspect

from alice.iot.dashboards.lib.solomon.alert_builder import Alert, AlertBuilderV2
from alice.iot.dashboards.lib.solomon.dashboard_builder import DashboardBuilder, Panel
from alice.iot.dashboards.lib.solomon.graphic_builder import RGBA
from alice.iot.dashboards.lib.solomon.provider import NamedMetric
from alice.iot.dashboards.lib.solomon.service_graphics import ServiceGraphics, HttpHandler
from alice.iot.dashboards.lib.solomon.aql import AQL


class UxieGraphics(ServiceGraphics):
    PROJECT = "alice-iot"
    CLUSTER = "uxie_production|uxie_beta"
    SERVICE = "uxie"

    PARAMETERS = [
        {
            "name": "project",
            "value": PROJECT,
        },
        {
            "name": "cluster",
            "value": CLUSTER,
        },
        {
            "name": "service",
            "value": SERVICE,
        },
        {
            "name": "host",
            "value": "*",
        }
    ]

    APPHOST_HANDLERS = [
        NamedMetric(name="/apphost/user/storage"),
        NamedMetric(name="/apphost/user/experiments"),
        NamedMetric(name="/apphost/user/devices"),
        NamedMetric(name="/apphost/user/info"),
    ]

    UXIE_HANDLERS = [
        HttpHandler(path="/v1.0/user/", method="GET"),
        HttpHandler(path="/v1.0/user/info", method="GET"),
        HttpHandler(path="/api/v1.0/user/info", method="GET"),
    ]

    CACHE_PARAMETERS = [
        {
            "name": "project",
            "value": PROJECT,
        },
        {
            "name": "cluster",
            "value": CLUSTER,
        },
        {
            "name": "service",
            "value": SERVICE,
        },
        {
            "name": "host",
            "value": "*",
        },
        {
            "name": "cache",
            "value": "pumpkin"
        }
    ]

    YDB_MM_INFO_METHODS = [
        "selectUser",
        "selectUserInfo",
        "selectAllExperiments",
    ]

    PERF_PARAMETERS = [
        {
            "name": "project",
            "value": PROJECT,
        },
        {
            "name": "cluster",
            "value": CLUSTER,
        },
        {
            "name": "service",
            "value": SERVICE,
        }
    ]

    DC = [
        NamedMetric("Sas", RGBA(r=155, g=100, b=0), False),
        NamedMetric("Man", RGBA(r=0, g=100, b=155), False),
        NamedMetric("Vla", RGBA(r=100, g=155, b=0), False),
    ]

    def update_graphics(self, project_id):
        self.update_apphost_graphics(project_id)
        self.update_service_graphics(project_id)
        self.update_ydb_cache_graphics(project_id)
        self.update_ydb_graphics(project_id)
        self.update_perf_graphics(project_id)
        # all other graphics reuse bulbasaur graphics

    def update_apphost_graphics(self, project_id):
        self.update(
            project_id=project_id,
            graphic_id="uxie_apphost_handlers_total",
            name="Uxie: Методы Apphost - RPS",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.ApphostGraphics.handler_total, self.APPHOST_HANDLERS)),
        )
        self.update(
            project_id=project_id,
            graphic_id="uxie_apphost_handlers_fails",
            name="Uxie: Методы Apphost - Ошибки, RPS",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.ApphostGraphics.handler_fails, self.APPHOST_HANDLERS)),
        )
        self.update(
            project_id=project_id,
            graphic_id="uxie_apphost_handlers_fails_percent",
            name="Uxie: Методы Apphost - Ошибки, %",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.ApphostGraphics.handler_fails_percent, self.APPHOST_HANDLERS)),
        )
        self.update(
            project_id=project_id,
            graphic_id="uxie_apphost_handlers_percentiles",
            name="Uxie: Методы Apphost - время, с (квантиль 99.9%)",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.ApphostGraphics.handler_percentile, self.APPHOST_HANDLERS)),
        )

    def update_service_graphics(self, project_id):
        self.update(
            project_id=project_id,
            graphic_id="uxie_uniproxy_handlers_rps",
            name="Uxie: получение данных УД – RPS",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.HttpGraphics.handler_rps, self.UXIE_HANDLERS)),
        )
        self.update(
            project_id=project_id,
            graphic_id="uxie_uniproxy_handlers_percentiles",
            name="Uxie: получение данных УД - время, с (квантиль 99.9%)",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.HttpGraphics.handler_percentile, self.UXIE_HANDLERS)),
        )
        self.update(
            project_id=project_id,
            graphic_id="uxie_uniproxy_handlers_errors_rps",
            name="Uxie: получение данных УД - RPS HTTP ошибок",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.HttpGraphics.handler_errors_rps, self.UXIE_HANDLERS)),
        )
        self.update(
            project_id=project_id,
            graphic_id="uxie_uniproxy_handlers_errors_percent",
            name="Uxie: получение данных УД - % HTTP ошибок",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.HttpGraphics.handler_errors_percent, self.UXIE_HANDLERS)),
            extra_data={
                "min": 0,
                "max": 100,
            }
        )

    def update_ydb_cache_graphics(self, project_id):
        self.update(
            project_id=project_id,
            graphic_id="uxie_ydb_cache_stats",
            name="Uxie: YDB Cache stats - %",
            parameters=self.CACHE_PARAMETERS,
            expressions=[
                ServiceGraphics.DBGraphics.cache_stat_rps("hit"),
                ServiceGraphics.DBGraphics.cache_stat_rps("miss"),
                ServiceGraphics.DBGraphics.cache_stat_rps("not_used"),
                ServiceGraphics.DBGraphics.cache_stat_rps("ignored"),
            ],
            extra_data={
                "normalize": True,
                "scale": "LOG",
                "min": 0,
                "max": 100,
            }
        )

    def update_ydb_graphics(self, project_id):
        self.update(
            project_id=project_id,
            graphic_id="uxie_ydb_megamind_userinfo_methods_rps",
            name="Uxie: Методы YDB ММ userinfo - RPS",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.DBGraphics.db_method_rps, self.YDB_MM_INFO_METHODS))
        )
        self.update(
            project_id=project_id,
            graphic_id="uxie_ydb_megamind_userinfo_methods_errors_rps",
            name="Uxie: методы YDB ММ userinfo - RPS Ошибок",
            parameters=self.PARAMETERS,
            expressions=list(
                map(
                    lambda method: ServiceGraphics.DBGraphics.db_method_errors_rps("ydb", method),
                    self.YDB_MM_INFO_METHODS
                )
            ),
        )
        self.update(
            project_id=project_id,
            graphic_id="uxie_ydb_megamind_userinfo_methods_percentile",
            name="Uxie: методы YDB MM userinfo - время, с (квантиль 99.9%)",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.DBGraphics.db_method_percentile, self.YDB_MM_INFO_METHODS)),
        )

    def update_perf_graphics(self, project_id):
        self.update(
            project_id=project_id,
            graphic_id="uxie_perf_goroutines_count",
            name="Uxie: Горутины - Кол-во",
            parameters=self.PERF_PARAMETERS,
            expressions=list(map(ServiceGraphics.PerfGraphics.goroutines_count, self.DC))
        )
        self.update(
            project_id=project_id,
            graphic_id="uxie_perf_memory_heap_in_use",
            name="Uxie: Heap - In use, Mb",
            parameters=self.PERF_PARAMETERS,
            expressions=list(
                map(lambda dc: ServiceGraphics.PerfGraphics.memtype_in_use(dc, "heap", 1024 * 1024), self.DC))
        )
        self.update(
            project_id=project_id,
            graphic_id="uxie_perf_memory_stack_in_use",
            name="Uxie: Stack - In use, Mb",
            parameters=self.PERF_PARAMETERS,
            expressions=list(
                map(lambda dc: ServiceGraphics.PerfGraphics.memtype_in_use(dc, "stack", 1024 * 1024), self.DC))
        )


class UxieDashboards(DashboardBuilder):
    PROJECT = "alice-iot"
    CLUSTER = "uxie_production|uxie_beta"
    SERVICE = "uxie"

    PARAMETERS = [
        {
            "name": "project",
            "value": PROJECT,
        },
        {
            "name": "cluster",
            "value": CLUSTER,
        },
        {
            "name": "service",
            "value": SERVICE,
        },
        {
            "name": "host",
            "value": "*",
        }
    ]

    def __init__(self, oauth_token):
        DashboardBuilder.__init__(self, oauth_token)

    def update_dashboards(self, project_id):
        self.update_service_dashboard(project_id)
        self.update_ydb_dashboard(project_id)
        # all other dashboards reuse bulbasaur graphics

    def update_ydb_dashboard(self, project_id):
        self.update(
            project_id=project_id,
            dashboard_id="uxie_ydb_dashboard",
            name="Uxie: YDB stats",
            parameters=self.PARAMETERS,
            rows=[
                [
                    Panel(
                        title="SessionPool",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "sensor", "db.stats.*_sessions",
                            "stack", "false",
                            "graph", "auto",
                            "legend", "off",
                            "checks", "-db.stats.max_sessions%3Bdb.stats.min_sessions",
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="SessionPoolBalance",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "sensor", "db.stats.pool_balance",
                            "stack", "false",
                            "graph", "auto",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="CPU Utilization in ThreadPool",
                        url="https://solomon.yandex-team.ru/?project=kikimr&cluster=ydb_ru&database=/ru/quasar/production/iotdb&service=utils&host=ydb-ru-*&slot=static&execpool=User&graph=ydb-execpool-utilization&legend=off",
                        rowspan=1,
                        colspan=6,
                    ),
                ],
                [
                    Panel(
                        title="Методы YDB MM userinfo - RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "uxie_ydb_megamind_userinfo_methods_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы YDB MM userinfo - RPS Ошибок",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "uxie_ydb_megamind_userinfo_methods_errors_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы YDB MM userinfo - время, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "uxie_ydb_megamind_userinfo_methods_percentile",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        type="MARKDOWN",
                        title="Соседние графики",
                        markdown="""+ [Cluster DB Status](https://solomon.yandex-team.ru/?project=kikimr&service=kqp&cluster=ydb_ru&database=/ru/quasar/production/iotdb&host=cluster&slot=cluster&dashboard=kikimr-mt-database-overall&b=30m&e=)

+ [YDB RPS по методам](https://solomon.yandex-team.ru/?project=alice-iot&cluster=uxie_production&service=uxie&host=cluster&db_method=selectUserInfo&sensor=db.total&graph=auto&legend=on)

+ [RPS ошибок по методам](https://solomon.yandex-team.ru/?project=alice-iot&cluster=uxie_production&service=uxie&host=cluster&db_method=selectUserInfo&sensor=db.fails&error_type=operation|transport|other&graph=auto&legend=off)

+ [Время ответа по методам](https://solomon.yandex-team.ru/?project=alice-iot&cluster=uxie_production&service=uxie&host=cluster&db_method=selectUserInfo&sensor=db.duration_buckets&overLinesTransform=WEIGHTED_PERCENTILE&percentiles=90%2C+95%2C+99%2C+99.9&graph=auto&legend=on)""",
                        rowspan=1,
                        colspan=3,
                    ),
                ],
                [
                    Panel(
                        title="TxLatencyWeightedPercentile",
                        url="?project=kikimr&cluster=ydb_ru&service=proxy&host=cluster&slot=cluster&sensor=TxTotalTimes&l.range=!INF ms&graph=auto&cs=gradient&checks=-&overLinesTransform=WEIGHTED_PERCENTILE&percentiles=50%2C90%2C95%2C99&l.database=/ru/quasar/production/iotdb&downsamplingAggr=max",
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="DataShard RangeRead Rows",
                        url="?project=kikimr&cluster=ydb_ru&service=tablets|slaves|slaves&host=cluster&slot=cluster&database=/ru/quasar/production/iotdb&sensor=DataShard%2FEngineHostRangeReadRows&type=DataShard&graph=auto&downsamplingAggr=max",
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="DataShard RangeRead Bytes",
                        url="?project=kikimr&cluster=ydb_ru&service=tablets|slaves&host=cluster&slot=cluster&database=/ru/quasar/production/iotdb&sensor=DataShard%2FEngineHostRangeReadBytes&type=DataShard&graph=auto&downsamplingAggr=max",
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="KQP Workers Count for DB",
                        url="?project=kikimr&cluster=ydb_ru&service=kqp&host=cluster&slot=cluster&graph=kikimr-kqp-workers-count-mt&l.database=%2Fru%2Fquasar%2Fproduction%2Fiotdb",
                        rowspan=1,
                        colspan=3,
                    ),
                ],
                [
                    Panel(
                        title="YQL Issues",
                        url="?project=kikimr&cluster=ydb_ru&service=kqp&l.host=cluster&l.sensor=Issues%2FYQL%3A*&l.database=/ru/quasar/production/iotdb&l.slot=cluster&graph=auto&downsamplingAggr=max&checks=-Issues%2FYQL%3ACORE_EXEC",
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="YQL Issues (KIKIMR_TEMPORARILY_UNAVAILABLE)",
                        url="?project=kikimr&cluster=ydb_ru&service=kqp&l.host=cluster&l.sensor=Issues%2FYQL%3AKIKIMR_TEMPORARILY_UNAVAILABLE&l.database=/ru/quasar/production/iotdb&l.slot=cluster&graph=auto&downsamplingAggr=max",
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Tasks in Resource Broker Queues",
                        url="?project=kikimr&cluster=ydb_ru&service=tablets&host=cluster&slot=cluster&database=/ru/quasar/production/iotdb&graph=auto&l.sensor=EnqueuedTasks&l.queue=!total&graph=auto",
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="TransactionsAborted",
                        url="?graph=auto&project=kikimr&service=kqp&host=cluster&slot=cluster&cluster=ydb_ru&l.sensor=Transactions%2FAborted&l.database=/ru/quasar/production/iotdb&downsamplingAggr=max",
                        rowspan=1,
                        colspan=3,
                    ),
                ]
            ]
        )

    def update_service_dashboard(self, project_id):
        self.update(
            project_id=project_id,
            dashboard_id="uxie_service_dashboard",
            name="Uxie: Service stats",
            parameters=self.PARAMETERS,
            rows=[
                [
                    Panel(
                        title="Получение пользовательской информации - RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "uxie_uniproxy_handlers_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=6,
                    ),
                    Panel(
                        title="Получение пользовательской информации - время, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "uxie_uniproxy_handlers_percentiles",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
                [
                    Panel(
                        title="Получение пользовательской информации - RPS HTTP Ошибок",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "uxie_uniproxy_handlers_errors_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Получение пользовательской информации - % HTTP Ошибок",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "uxie_uniproxy_handlers_errors_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="YDB Cache - % Stats",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "cache", "pumpkin",
                            "graph", "uxie_ydb_cache_stats",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
                [
                    Panel(
                        title="Методы Apphost - RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "uxie_apphost_handlers_total",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=6,
                    ),
                    Panel(
                        title="Методы Apphost - время, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "uxie_apphost_handlers_percentiles",
                            "legend", "off"
                        ),
                        rowspan=2,
                        colspan=3,
                    ),
                ],
                [
                    Panel(
                        title="Методы Apphost - RPS Ошибок",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "uxie_apphost_handlers_fails",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы Apphost - % Ошибок",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "uxie_apphost_handlers_fails_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
                [
                    Panel(
                        title="HTTP handlers panics",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "sensor", "{{handlers.fails}}",
                            "graph", "auto",
                            "legend", "off",
                            "l.sensor", "handlers.fails",
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Apphost handlers panics",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "sensor", "{{apphost.fails}}",
                            "graph", "auto",
                            "legend", "off",
                            "l.sensor", "handlers.fails",
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        type="MARKDOWN",
                        title="Соседние графики",
                        markdown="""+ [Балансер](https://yasm.yandex-team.ru/template/panel/balancer_common_panel/fqdn=iot.quasar.yandex.net;itype=balancer;ctype=prod;metaprj=balancer;locations=man,sas,vla;prj=quasar-iot,quasar-iot-internal;signal=uxie/)

+ [Uniproxy context load](https://yasm.yandex-team.ru/template/panel/voiceinfra_context_load/project=uniproxy;environment=prod;locations=sas-pre,sas,man,vla;components=IOT_USER_INFO/)""",
                        rowspan=1,
                        colspan=3,
                    ),
                ],
                [
                    Panel(
                        title="Heap - In use, mb",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "graph", "uxie_perf_memory_heap_in_use",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Stack - In use, mb",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "graph", "uxie_perf_memory_stack_in_use",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Горутины - Кол-во",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "host", "cluster",
                            "service", "{{service}}",
                            "graph", "uxie_perf_goroutines_count",
                            "legend", "off",
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
            ]
        )


class UxieAlerts(AlertBuilderV2):
    def __init__(self, oauth_token):
        AlertBuilderV2.__init__(self, oauth_token)

    CHANNELS = [
        {
            "id": "bulbasaur_juggler_channel",
            "config":
                {
                    "notifyAboutStatuses": [
                        "ALARM",
                        "OK",
                        "WARN",
                        "ERROR",
                    ]
                },
        },
    ]

    PROJECT = "alice-iot"
    CLUSTER = "uxie_production"
    SERVICE = "uxie"

    PARAMETERS = [
        {
            "name": "project",
            "value": PROJECT,
        },
        {
            "name": "cluster",
            "value": CLUSTER,
        },
        {
            "name": "service",
            "value": SERVICE,
        },
        {
            "name": "host",
            "value": "*",
        }
    ]

    APPHOST_HANDLERS = [
        NamedMetric(name="/apphost/user/storage"),
        NamedMetric(name="/apphost/user/experiments"),
        NamedMetric(name="/apphost/user/devices"),
        NamedMetric(name="/apphost/user/info"),
    ]

    def update_alerts(self, project_id):
        self.update_apphost_alerts(project_id)

    def update_apphost_alerts(self, project_id):
        self.update_apphost_handlers_fails_rps_alert(project_id)
        self.update_apphost_select_user_info_time_alert(project_id)

    def update_apphost_handlers_fails_rps_alert(self, project_id):
        alert_id = "uxie_apphost_handlers_fails_rps"
        evaluation_window = 15
        alert = self.uxie_apphost_handlers_fails_rps_alert(
            target=1,
            alert_id=alert_id
        )

        self.update(
            project_id=project_id,
            alert_id=alert_id,
            name="Uxie: apphost handlers fails RPS",
            alert=alert,
            channels=self.CHANNELS,
            window_secs=evaluation_window,
        )

    def uxie_apphost_handlers_fails_rps_alert(self, target, alert_id):
        path = "|".join([metric.name for metric in self.APPHOST_HANDLERS])
        selector = f'{{sensor="apphost.fails", path="{path}", cluster="{self.CLUSTER}", ' \
                   f'host="cluster", service="{self.SERVICE}"}}'

        max_series = AQL.combinate_expression("series_max", selector)
        max_value = AQL.max(max_series)
        expression = AQL.alert_if_gt(max_value, target)

        notification_message = inspect.cleandoc(
            f"""
                Status: {{{{status.code}}}}, value: {{{{expression.{AQL.current_value_variable_name}}}}}

                Graph: https://solomon.yandex-team.ru/?project=alice-iot&cluster=uxie_production&service=uxie&host=cluster&graph=uxie_apphost_handlers_fails

                Solomon alert: https://solomon.yandex-team.ru/admin/projects/alice-iot/alerts/{alert_id}
            """
        )

        annotations = {
            'notification_message': notification_message
        }

        return Alert(
            program=expression,
            annotations=annotations,
        )

    def update_apphost_select_user_info_time_alert(self, project_id):
        alert_id = "uxie_select_user_info_time_95"
        evaluation_window = 60

        alert = self.uxie_apphost_select_user_info_time_alert(
            alert_id=alert_id,
            target_warn=0.05,
            target_crit=0.1,
            crit_points_number_to_alarm=3,
        )

        self.update(
            project_id=project_id,
            alert_id=alert_id,
            name="Uxie: select user info time 95%",
            alert=alert,
            channels=self.CHANNELS,
            window_secs=evaluation_window,
            delay_secs=30,
        )

    def uxie_apphost_select_user_info_time_alert(self, alert_id, target_warn, target_crit, crit_points_number_to_alarm):
        warn_points_num_name = 'warn_points_num'
        crit_points_num_name = 'crit_points_num'
        avg_value_name = 'avg_value'

        expression = f'let {warn_points_num_name} = count(drop_nan(drop_below(histogram_percentile(95, {{sensor="db.duration_buckets", cluster="{self.CLUSTER}", service="{self.SERVICE}", host="cluster", bin!="inf", db_method="selectUserInfo"}}), {target_warn})));\n\n' \
                     f'let {crit_points_num_name} = count(drop_nan(drop_below(histogram_percentile(95, {{sensor="db.duration_buckets", cluster="{self.CLUSTER}", service="{self.SERVICE}", host="cluster", bin!="inf", db_method="selectUserInfo"}}), {target_crit})));\n\n' \
                     f'let {avg_value_name} = avg(histogram_percentile(95, {{sensor="db.duration_buckets", cluster="uxie_production", service="uxie", host="cluster", bin!="inf", db_method="selectUserInfo"}}));\n\n' \
                     f'alarm_if({crit_points_num_name} >= {crit_points_number_to_alarm});\n' \
                     f'warn_if({warn_points_num_name} >= {crit_points_number_to_alarm});'

        notification_message = inspect.cleandoc(
            f'''
                Status: {{{{status.code}}}}, average percentile value: {{{{expression.{avg_value_name}}}}}, points >= {target_crit}s: {{{{expression.{crit_points_num_name}}}}}, points >= {target_warn}s: {{{{expression.{warn_points_num_name}}}}}

                Graph: https://solomon.yandex-team.ru/?project=alice-iot&cluster=uxie_production&service=uxie&host=cluster&db_method=selectUserInfo&sensor=db.duration_buckets&overLinesTransform=WEIGHTED_PERCENTILE&percentiles=90%2C+95%2C+99%2C+99.9&graph=auto&legend=on&b=1h&e=

                Solomon alert: https://solomon.yandex-team.ru/admin/projects/alice-iot/alerts/{alert_id}
            '''
        )

        annotations = {
            'notification_message': notification_message
        }

        return Alert(
            program=expression,
            annotations=annotations,
        )
