# -*- coding: utf8 -*-
# flake8: noqa
"""
Xiaomi graphics
"""

from alice.iot.dashboards.lib.solomon.dashboard_builder import DashboardBuilder, Panel
from alice.iot.dashboards.lib.solomon.graphic_builder import RGBA
from alice.iot.dashboards.lib.solomon.provider import NamedMetric
from alice.iot.dashboards.lib.solomon.queue_graphics import QueueGraphics
from alice.iot.dashboards.lib.solomon.service_graphics import ServiceGraphics


class XiaomiGraphics(ServiceGraphics):
    PROJECT = "alice-iot"
    CLUSTER = "xiaomi_producton|xiaomi_beta"
    SERVICE = "xiaomi"

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
    IOT_API_PARAMETERS = [
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
            "name": "region",
            "value": "*"
        }
    ]
    XIAOMI_QUEUE_PARAMETERS = [
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
            "name": "task",
            "value": "*"
        }
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

    IOT_API_CALLS = [
        NamedMetric(name="get_user_devices"),
        NamedMetric(name="get_user_homes"),
        NamedMetric(name="get_user_device_info"),
        NamedMetric(name="get_properties"),
        NamedMetric(name="set_properties"),
        NamedMetric(name="set_actions"),
    ]
    MIOTSPEC_API_CALLS = [
        NamedMetric(name="get_device_services"),
    ]
    USER_API_CALLS = [
        NamedMetric(name="get_user_profile"),
    ]

    CODES = {
        "4xx": NamedMetric(name="4xx", color=RGBA(r=255, g=153, b=0, a=0.5)),
        "5xx": NamedMetric(name="5xx", color=RGBA(r=255, g=0, b=0)),
        "fails": NamedMetric(name="unavailable", color=RGBA(r=100, g=50, b=0)),
        "2xx": NamedMetric(name="2xx", color=RGBA(r=64, g=255, b=64, a=0.3)),
    }

    TASK_STATUSES = [
        NamedMetric("processed", RGBA(r=155, g=100, b=0), False),
        NamedMetric("ready", RGBA(r=0, g=100, b=155), False),
        NamedMetric("running", RGBA(r=100, g=155, b=0), False),
        NamedMetric("done"),
        NamedMetric("failed", )
    ]

    PGDB_SUBSCRIPTION_METHODS = [
        "storeDeviceSubscriptions",
        "storeUserSubscriptions"
    ]

    PGDB_USER_METHODS = [
        "selectExternalUser",
        "storeExternalUser",
        "deleteExternalUser"
    ]

    CALLBACK_METHODS = [
        "userEvent",
        "propertiesChanged",
    ]

    def __init__(self, oauth_token):
        ServiceGraphics.__init__(self, oauth_token)

    def update_graphics(self, project_id):
        # self.update_api_graphics(project_id)
        self.update_queue_graphics(project_id)
        self.update_pgdb_graphics(project_id)
        self.update_callback_graphics(project_id)
        # self.update_perf_graphics(project_id)

    def update_pgdb_graphics(self, project_id):
        # User PGDB Methods
        self.update(
            project_id=project_id,
            graphic_id="xiaomi_pgdb_user_methods_rps",
            name="Xiaomi: Методы PGDB User - RPS",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.DBGraphics.db_method_rps, self.PGDB_USER_METHODS))
        )
        self.update(
            project_id=project_id,
            graphic_id="xiaomi_pgdb_user_methods_errors_rps",
            name="Xiaomi: Методы PGDB User - RPS Ошибок",
            parameters=self.PARAMETERS,
            expressions=list(
                map(
                    lambda method: ServiceGraphics.DBGraphics.db_method_errors_rps("pgdb", method),
                    self.PGDB_USER_METHODS
                )
            ),
            extra_data={
                "min": 0,
                "max": 1
            }
        )
        self.update(
            project_id=project_id,
            graphic_id="xiaomi_pgdb_user_methods_percentile",
            name="Xiaomi: Методы PGDB User - время, с (квантиль 99.9%)",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.DBGraphics.db_method_percentile, self.PGDB_USER_METHODS)),
            extra_data={
                "min": 0,
                "max": 5,
            }
        )

        # Subscription PGDB Methods
        self.update(
            project_id=project_id,
            graphic_id="xiaomi_pgdb_subscription_methods_rps",
            name="Xiaomi: Методы PGDB Subscription - RPS",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.DBGraphics.db_method_rps, self.PGDB_SUBSCRIPTION_METHODS))
        )
        self.update(
            project_id=project_id,
            graphic_id="xiaomi_pgdb_subscription_methods_errors_rps",
            name="Xiaomi: Методы PGDB Subscription - RPS Ошибок",
            parameters=self.PARAMETERS,
            expressions=list(
                map(
                    lambda method: ServiceGraphics.DBGraphics.db_method_errors_rps("pgdb", method),
                    self.PGDB_SUBSCRIPTION_METHODS
                )
            ),
            extra_data={
                "min": 0,
                "max": 1
            }
        )
        self.update(
            project_id=project_id,
            graphic_id="xiaomi_pgdb_subscription_methods_percentile",
            name="Xiaomi: Методы PGDB Subscription - время, с (квантиль 99.9%)",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.DBGraphics.db_method_percentile, self.PGDB_SUBSCRIPTION_METHODS)),
            extra_data={
                "min": 0,
                "max": 5,
            }
        )

    def update_callback_graphics(self, project_id):
        self.update(
            project_id=project_id,
            graphic_id="xiaomi_callback_methods_rps",
            name="Xiaomi: Callbacks - RPS",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.CallbackGraphics.callback_method_rps, self.CALLBACK_METHODS))
        )
        self.update(
            project_id=project_id,
            graphic_id="xiaomi_callback_methods_errors_rps",
            name="Xiaomi: Callbacks - RPS Ошибок",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.CallbackGraphics.callback_method_errors_rps, self.CALLBACK_METHODS)),
            extra_data={
                "min": 0,
                "max": 1
            }
        )
        self.update(
            project_id=project_id,
            graphic_id="xiaomi_callback_methods_percentile",
            name="Xiaomi: Callbacks - время, с (квантиль 99.9%)",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.CallbackGraphics.callback_method_percentile, self.CALLBACK_METHODS)),
            extra_data={
                "min": 0,
                "max": 5,
            }
        )

    def update_queue_graphics(self, project_id):
        self.update(
            project_id=project_id,
            graphic_id="xiaomi_queue_task_processed",
            name="Xiaomi: Количество обработанных задач",
            parameters=self.XIAOMI_QUEUE_PARAMETERS,
            expressions=[
                QueueGraphics.task_status(NamedMetric(
                    name="failed", color=RGBA(r=200, g=0, b=0, a=0.8), stack=True, area=True,
                )),
                QueueGraphics.task_status(NamedMetric(
                    name="done", color=RGBA(r=10, g=255, b=10, a=0.4), stack=True, area=True,
                )),
            ],
        )
        self.update(
            project_id=project_id,
            graphic_id="xiaomi_queue_task_status_bars",
            name="Xiaomi: Количество задач по статусам",
            parameters=self.XIAOMI_QUEUE_PARAMETERS,
            expressions=[
                QueueGraphics.task_status(NamedMetric(
                    name="ready", color=RGBA(r=27, g=108, b=168, a=0.8),
                )),
                QueueGraphics.task_status(NamedMetric(
                    name="running", color=RGBA(r=252, g=232, b=213, a=0.8),
                )),
                QueueGraphics.task_status(NamedMetric(
                    name="failed", color=RGBA(r=200, g=0, b=0, a=0.8),
                )),
            ],
            extra_data={
                "graphMode": "BARS",
                "aggr": "LAST",
            }
        )
        self.update(
            project_id=project_id,
            graphic_id="xiaomi_queue_task_timings",
            name="Xiaomi: Обработка задачи - время, мс",
            parameters=self.XIAOMI_QUEUE_PARAMETERS,
            expressions=[
                QueueGraphics.average_overdue_time(),
                QueueGraphics.average_process_time()
            ]
        )

    def update_api_graphics(self, project_id):
        self.update_iot_api_graphics(project_id)
        self.update_miotspec_api_graphics(project_id)
        self.update_user_api_graphics(project_id)

    def update_iot_api_graphics(self, project_id):
        self.update(
            project_id=project_id,
            graphic_id="xiaomi_iot_api_calls_rps",
            name="Xiaomi: Методы IotAPI - RPS",
            parameters=self.IOT_API_PARAMETERS,
            expressions=list(map(ServiceGraphics.ApiCallGraphics.api_call_rps, self.IOT_API_CALLS))
        )
        self.update(
            project_id=project_id,
            graphic_id="xiaomi_iot_api_calls_errors",
            name="Xiaomi: Методы IotAPI - Ошибки, %",
            parameters=self.IOT_API_PARAMETERS,
            expressions=[
                ServiceGraphics.ApiCallGraphics.api_call_total_http_code_percent(self.CODES["4xx"], "iot"),
                ServiceGraphics.ApiCallGraphics.api_call_total_http_code_percent(self.CODES["5xx"], "iot"),
                ServiceGraphics.ApiCallGraphics.api_call_total_unavailable_percent(self.CODES["fails"], "iot"),
            ],
            extra_data={
                "min": 0,
                "max": 110,
            }
        )
        self.update(
            project_id=project_id,
            graphic_id="xiaomi_iot_api_calls_errors_rps",
            name="Xiaomi: Методы IotAPI - Ошибки, RPS",
            parameters=self.IOT_API_PARAMETERS,
            expressions=list(map(ServiceGraphics.ApiCallGraphics.api_call_errors_rps, self.IOT_API_CALLS)),
            extra_data={
                "min": 0,
                "max": 110,
            }
        )
        self.update(
            project_id=project_id,
            graphic_id="xiaomi_iot_api_calls_errors_percent",
            name="Xiaomi: Методы IotAPI - Ошибки, %",
            parameters=self.IOT_API_PARAMETERS,
            expressions=list(map(ServiceGraphics.ApiCallGraphics.api_call_errors_percent, self.IOT_API_CALLS)),
            extra_data={
                "min": 0,
                "max": 110,
            }
        )
        self.update(
            project_id=project_id,
            graphic_id="xiaomi_iot_api_calls_percentiles",
            name="Xiaomi: Методы IotAPI - время, с (квантиль 99.9%)",
            parameters=self.IOT_API_PARAMETERS,
            expressions=list(map(ServiceGraphics.ApiCallGraphics.api_call_percentile, self.IOT_API_CALLS)),
            extra_data={
                "min": 0,
                "max": 5,
            }
        )

    def update_miotspec_api_graphics(self, project_id):
        self.update(
            project_id=project_id,
            graphic_id="xiaomi_miotspec_api_calls_rps",
            name="Xiaomi: Методы MiotspecAPI - RPS",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.ApiCallGraphics.api_call_rps, self.MIOTSPEC_API_CALLS))
        )
        self.update(
            project_id=project_id,
            graphic_id="xiaomi_miotspec_api_calls_errors",
            name="Xiaomi: Методы MiotspecAPI - Ошибки, %",
            parameters=self.PARAMETERS,
            expressions=[
                ServiceGraphics.ApiCallGraphics.api_call_total_http_code_percent(self.CODES["4xx"], "miotspec"),
                ServiceGraphics.ApiCallGraphics.api_call_total_http_code_percent(self.CODES["5xx"], "miotspec"),
                ServiceGraphics.ApiCallGraphics.api_call_total_unavailable_percent(self.CODES["fails"], "miotspec"),
            ],
            extra_data={
                "min": 0,
                "max": 110,
            }
        )
        self.update(
            project_id=project_id,
            graphic_id="xiaomi_miotspec_api_calls_errors_rps",
            name="Xiaomi: Методы MiotspecAPI - Ошибки, RPS",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.ApiCallGraphics.api_call_errors_rps, self.MIOTSPEC_API_CALLS)),
            extra_data={
                "min": 0,
                "max": 110,
            }
        )
        self.update(
            project_id=project_id,
            graphic_id="xiaomi_miotspec_api_calls_errors_percent",
            name="Xiaomi: Методы MiotspecAPI - Ошибки, %",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.ApiCallGraphics.api_call_errors_percent, self.MIOTSPEC_API_CALLS)),
            extra_data={
                "min": 0,
                "max": 110,
            }
        )
        self.update(
            project_id=project_id,
            graphic_id="xiaomi_miotspec_api_calls_percentiles",
            name="Xiaomi: Методы MiotspecAPI - время, с (квантиль 99.9%)",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.ApiCallGraphics.api_call_percentile, self.MIOTSPEC_API_CALLS)),
            extra_data={
                "min": 0,
                "max": 5,
            }
        )

    def update_user_api_graphics(self, project_id):
        self.update(
            project_id=project_id,
            graphic_id="xiaomi_user_api_calls_rps",
            name="Xiaomi: Методы UserAPI - RPS",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.ApiCallGraphics.api_call_rps, self.USER_API_CALLS))
        )
        self.update(
            project_id=project_id,
            graphic_id="xiaomi_user_api_calls_errors",
            name="Xiaomi: Методы UserAPI - Ошибки, %",
            parameters=self.PARAMETERS,
            expressions=[
                ServiceGraphics.ApiCallGraphics.api_call_total_http_code_percent(self.CODES["4xx"], "user"),
                ServiceGraphics.ApiCallGraphics.api_call_total_http_code_percent(self.CODES["5xx"], "user"),
                ServiceGraphics.ApiCallGraphics.api_call_total_unavailable_percent(self.CODES["fails"], "user"),
            ],
            extra_data={
                "min": 0,
                "max": 110,
            }
        )
        self.update(
            project_id=project_id,
            graphic_id="xiaomi_user_api_calls_errors_rps",
            name="Xiaomi: Методы UserAPI - Ошибки, RPS",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.ApiCallGraphics.api_call_errors_rps, self.USER_API_CALLS)),
            extra_data={
                "min": 0,
                "max": 110,
            }
        )
        self.update(
            project_id=project_id,
            graphic_id="xiaomi_user_api_calls_errors_percent",
            name="Xiaomi: Методы UserAPI - Ошибки, %",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.ApiCallGraphics.api_call_errors_percent, self.USER_API_CALLS)),
            extra_data={
                "min": 0,
                "max": 110,
            }
        )
        self.update(
            project_id=project_id,
            graphic_id="xiaomi_user_api_calls_percentiles",
            name="Xiaomi: Методы UserAPI - время, с (квантиль 99.9%)",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.ApiCallGraphics.api_call_percentile, self.USER_API_CALLS)),
            extra_data={
                "min": 0,
                "max": 5,
            }
        )

    def update_perf_graphics(self, project_id):
        self.update_perf_general_graphics(project_id)
        self.update_perf_gc_graphics(project_id)
        self.update_perf_memory_graphics(project_id)

    def update_perf_general_graphics(self, project_id):
        self.update(
            project_id=project_id,
            graphic_id="xiaomi_perf_goroutines_count",
            name="Xiaomi: Горутины - Кол-во",
            parameters=self.PERF_PARAMETERS,
            expressions=list(map(ServiceGraphics.PerfGraphics.goroutines_count, self.DC))
        )
        self.update(
            project_id=project_id,
            graphic_id="xiaomi_perf_total_sys_alloc",
            name="Xiaomi: Total sys - alloc, Mb",
            parameters=self.PERF_PARAMETERS,
            expressions=list(map(lambda dc: ServiceGraphics.PerfGraphics.total_sys_alloc(dc, 1024 * 1024), self.DC))
        )

    def update_perf_gc_graphics(self, project_id):
        self.update(
            project_id=project_id,
            graphic_id="xiaomi_perf_gc_count",
            name="Xiaomi: Циклы GC - Кол-во",
            parameters=self.PERF_PARAMETERS,
            expressions=list(map(ServiceGraphics.PerfGraphics.gc_count, self.DC)),
            extra_data={
                "min": "0",
            }
        )
        self.update(
            project_id=project_id,
            graphic_id="xiaomi_perf_gc_pause",
            name="Xiaomi: Циклы GC - Время, мс",
            parameters=self.PERF_PARAMETERS,
            expressions=list(map(ServiceGraphics.PerfGraphics.gc_pauses, self.DC)),
            extra_data={
                "min": "0",
            }
        )

    def update_perf_memory_graphics(self, project_id):
        self.update(
            project_id=project_id,
            graphic_id="xiaomi_perf_memory_heap_alloc",
            name="Xiaomi: Heap - Alloc, Mb",
            parameters=self.PERF_PARAMETERS,
            expressions=list(map(lambda dc: ServiceGraphics.PerfGraphics.heap_alloc(dc, 1024 * 1024), self.DC))
        )
        self.update(
            project_id=project_id,
            graphic_id="xiaomi_perf_memory_heap_objects",
            name="Xiaomi: Heap objects - Кол-во",
            parameters=self.PERF_PARAMETERS,
            expressions=list(map(ServiceGraphics.PerfGraphics.heap_objects, self.DC))
        )
        self.update(
            project_id=project_id,
            graphic_id="xiaomi_perf_memory_heap_released",
            name="Xiaomi: Heap - Released, Mb",
            parameters=self.PERF_PARAMETERS,
            expressions=list(map(lambda dc: ServiceGraphics.PerfGraphics.heap_released(dc, 1024 * 1024), self.DC))
        )
        self.update(
            project_id=project_id,
            graphic_id="xiaomi_perf_memory_heap_in_use",
            name="Xiaomi: Heap - In use, Mb",
            parameters=self.PERF_PARAMETERS,
            expressions=list(
                map(lambda dc: ServiceGraphics.PerfGraphics.memtype_in_use(dc, "heap", 1024 * 1024), self.DC))
        )
        self.update(
            project_id=project_id,
            graphic_id="xiaomi_perf_memory_stack_in_use",
            name="Xiaomi: Stack - In use, Mb",
            parameters=self.PERF_PARAMETERS,
            expressions=list(
                map(lambda dc: ServiceGraphics.PerfGraphics.memtype_in_use(dc, "stack", 1024 * 1024), self.DC))
        )


class XiaomiDashboards(DashboardBuilder):
    PROJECT = "alice-iot"
    CLUSTER = "xiaomi_production|xiaomi_beta"
    SERVICE = "xiaomi"

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
        },
    ]
    IOT_API_PARAMETERS = [
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
            "name": "region",
            "value": "*"
        }
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

    def __init__(self, oauth_token):
        DashboardBuilder.__init__(self, oauth_token)

    def update_dashboards(self, project_id):
        # self.update_api_dashboard(project_id)
        # self.update_pgdb_dashboard(project_id)
        self.update_callback_dashboard(project_id)
        # self.update_queue_dashboard(project_id)
        # self.update_perf_dashboard(project_id)

    def update_api_dashboard(self, project_id):
        for api in ["iot", "miotspec", "user"]:
            self.update_xiaomi_api_dashboard(project_id, api)

    def update_pgdb_dashboard(self, project_id):
        self.update(
            project_id=project_id,
            dashboard_id="xiaomi_pgdb_dashboard",
            name="Xiaomi: PGDB stats",
            parameters=self.PARAMETERS,
            rows=[
                [
                    Panel(
                        title="Методы PGDB User - RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "xiaomi_pgdb_user_methods_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                    Panel(
                        title="Методы PGDB User - RPS Ошибок",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "xiaomi_pgdb_user_methods_errors_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                    Panel(
                        title="Методы PGDB User - время, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "xiaomi_pgdb_user_methods_percentile",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                ],
                [
                    Panel(
                        title="Методы PGDB Subscription - RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "xiaomi_pgdb_subscription_methods_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                    Panel(
                        title="Методы PGDB Subscription - RPS Ошибок",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "xiaomi_pgdb_subscription_methods_errors_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                    Panel(
                        title="Методы PGDB Subscription - время, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "xiaomi_pgdb_subscription_methods_percentile",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                ],
            ]
        )

    def update_callback_dashboard(self, project_id):
        self.update(
            project_id=project_id,
            dashboard_id="xiaomi_callback_dashboard",
            name="Xiaomi: Callback stats",
            parameters=self.PARAMETERS,
            rows=[
                [
                    Panel(
                        title="Callbacks - RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "xiaomi_callback_methods_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Callbacks - RPS Ошибок",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "xiaomi_callback_methods_errors_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Callbacks - время, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "xiaomi_callback_methods_percentile",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ],
                [
                    Panel(
                        title="PropertiesChanged - RPS Ошибок обработки колбека",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "sensor", "callback.fails",
                            "error_type", "handle",
                            "callback", "propertiesChanged",
                            "graph", "auto",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="UserEvent - RPS Ошибок обработки колбека",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "sensor", "callback.fails",
                            "error_type", "handle",
                            "callback", "userEvent",
                            "graph", "auto",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
            ]
        )

    def update_xiaomi_api_dashboard(self, project_id, xiaomi_api):
        def get_parameters(api):
            if api == "iot":
                return self.IOT_API_PARAMETERS
            return self.PARAMETERS

        self.update(
            project_id=project_id,
            dashboard_id="xiaomi_{api}_api_dashboard".format(api=xiaomi_api.lower()),
            name="Xiaomi: {api}API stats".format(api=xiaomi_api.capitalize()),
            parameters=get_parameters(xiaomi_api),
            rows=[
                [
                    Panel(
                        title="Методы {api}API - RPS".format(api=xiaomi_api.capitalize()),
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "region", "{{region}}",
                            "graph", "xiaomi_{api}_api_calls_rps".format(api=xiaomi_api.lower()),
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы {api}API - Всего ошибок, %".format(api=xiaomi_api.capitalize()),
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "region", "{{region}}",
                            "graph", "xiaomi_{api}_api_calls_errors".format(api=xiaomi_api.lower()),
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы {api}API - Тайминги, с (квантиль 99.9%)".format(api=xiaomi_api.capitalize()),
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "region", "{{region}}",
                            "graph", "xiaomi_{api}_api_calls_percentiles".format(api=xiaomi_api.lower()),
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ],
                [
                    Panel(
                        title="Методы {api}API - Ошибки, RPS".format(api=xiaomi_api.capitalize()),
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "region", "{{region}}",
                            "graph", "xiaomi_{api}_api_calls_errors_rps".format(api=xiaomi_api.lower()),
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы {api}API - Ошибки, %".format(api=xiaomi_api.capitalize()),
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "region", "{{region}}",
                            "graph", "xiaomi_{api}_api_calls_errors_percent".format(api=xiaomi_api.lower()),
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
            ]
        )

    def update_queue_dashboard(self, project_id):
        rows = []
        for task in ["PropertySubscriptionTask", "UserEventSubscriptionTask", "DeviceStatusSubscriptionTask"]:
            row = [
                Panel(
                    title="Количество обработанных задач, {task}".format(task=task),
                    url=DashboardBuilder.url_query(
                        "project", project_id,
                        "cluster", "{{cluster}}",
                        "service", "{{service}}",
                        "host", "{{host}}",
                        "task", task,
                        "graph", "xiaomi_queue_task_processed",
                        "legend", "off"
                    ),
                    rowspan=1,
                    colspan=1,
                ),
                Panel(
                    title="Количество задач по статусам, {task}".format(task=task),
                    url=DashboardBuilder.url_query(
                        "project", project_id,
                        "cluster", "{{cluster}}",
                        "service", "{{service}}",
                        "host", "{{host}}",
                        "task", task,
                        "graph", "xiaomi_queue_task_status_bars",
                        "legend", "off"
                    ),
                    rowspan=1,
                    colspan=1,
                ),
                Panel(
                    title="Обработка задачи, {task} - время, мс".format(task=task),
                    url=DashboardBuilder.url_query(
                        "project", project_id,
                        "cluster", "{{cluster}}",
                        "service", "{{service}}",
                        "host", "{{host}}",
                        "task", task,
                        "graph", "xiaomi_queue_task_timings",
                        "legend", "off"
                    ),
                    rowspan=1,
                    colspan=2,
                ),
            ]
            rows.append(row)
        self.update(
            project_id=project_id,
            dashboard_id="xiaomi_queue_dashboard",
            name="Xiaomi: Queue stats",
            parameters=self.PARAMETERS,
            rows=rows,
        )

    def update_perf_dashboard(self, project_id):
        self.update(
            project_id=project_id,
            dashboard_id="xiaomi_perf_dashboard",
            name="Xiaomi: Perf stats",
            parameters=self.PERF_PARAMETERS,
            rows=[
                [
                    Panel(
                        title="Total sys - Alloc, mb",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "graph", "xiaomi_perf_total_sys_alloc",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Циклы GC - Кол-во",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "graph", "xiaomi_perf_gc_count",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                    Panel(
                        title="Циклы GC - Время, мс",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "graph", "xiaomi_perf_gc_pause",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                    Panel(
                        title="Горутины - Кол-во",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "graph", "xiaomi_perf_goroutines_count",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                ],
                [
                    Panel(
                        title="Heap - In use, mb",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "graph", "xiaomi_perf_memory_heap_in_use",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                    Panel(
                        title="Heap - Alloc, mb",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "graph", "xiaomi_perf_memory_heap_alloc",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                    Panel(
                        title="Heap - Released, mb",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "graph", "xiaomi_perf_memory_heap_released",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                    Panel(
                        title="Heap objects - Кол-во",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "graph", "xiaomi_perf_memory_heap_objects",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                    Panel(
                        title="Stack - In use, mb",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "graph", "xiaomi_perf_memory_stack_in_use",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                ],
            ]
        )
