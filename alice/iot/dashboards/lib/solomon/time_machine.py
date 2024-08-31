# -*- coding: utf8 -*-
# flake8: noqa
"""
Time machine graphics
"""
from alice.iot.dashboards.lib.solomon.service_graphics import ServiceGraphics, HttpHandler
from alice.iot.dashboards.lib.solomon.dashboard_builder import DashboardBuilder, Panel


class TimeMachineGraphics(ServiceGraphics):
    PROJECT = "alice-iot"
    CLUSTER = "timemachine_producton|timemachine_beta"
    SERVICE = "timemachine"

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
    PGDB_QUEUE_PARAMETERS = [
        {
            "name": "project",
            "value": PROJECT,
        },
        {
            "name": "cluster",
            "value": "*",
        },
        {
            "name": "service",
            "value": "*",
        },
        {
            "name": "host",
            "value": "*",
        }
    ]
    API_HANDLERS = [
        HttpHandler(path="/v1.0/submit", method="POST"),
    ]
    PGDB_QUEUE_METHODS = [
        "submitTasks",
        "getTasksAndUpdateState",
        "getLostTasksAndUpdateState",
        "setTasksReady",
        "setTasksDone",
        "setTasksDoneAndSubmit",
        "setTasksFailed",
        "removeOldFinishedTasks",
    ]

    def __init__(self, oauth_token):
        ServiceGraphics.__init__(self, oauth_token)

    def update_graphics(self, project_id):
        #self.update_api_graphics(project_id)
        self.update_pgdb_queue_graphics(project_id)

    def update_api_graphics(self, project_id):
        self.update(
            project_id=project_id,
            graphic_id="timemachine_api_handlers_rps",
            name="Timemachine: Методы API - RPS",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.HttpGraphics.handler_rps, self.API_HANDLERS))
        )
        self.update(
            project_id=project_id,
            graphic_id="timemachine_api_handlers_errors_rps",
            name="Timemachine: Методы API - RPS HTTP ошибок",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.HttpGraphics.handler_errors_rps, self.API_HANDLERS)),
            extra_data={
                "min": 0,
                "max": 110,
            }
        )
        self.update(
            project_id=project_id,
            graphic_id="timemachine_api_handlers_errors_percent",
            name="Timemachine: Методы API - % HTTP ошибок",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.HttpGraphics.handler_errors_percent, self.API_HANDLERS)),
            extra_data={
                "min": 0,
                "max": 110,
            }
        )
        self.update(
            project_id=project_id,
            graphic_id="timemachine_api_handlers_percentiles",
            name="Timemachine: Методы API - время, с (квантиль 99.9%)",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.HttpGraphics.handler_percentile, self.API_HANDLERS)),
            extra_data={
                "min": 0,
                "max": 5,
            }
        )

    def update_pgdb_queue_graphics(self, project_id):
        self.update(
            project_id=project_id,
            graphic_id="pgdb_queue_methods_rps",
            name="Queue: Методы PGDB - RPS",
            parameters=self.PGDB_QUEUE_PARAMETERS,
            expressions=list(map(ServiceGraphics.DBGraphics.db_method_rps, self.PGDB_QUEUE_METHODS))
        )
        self.update(
            project_id=project_id,
            graphic_id="pgdb_queue_methods_errors_rps",
            name="Queue: Методы PGDB - RPS Ошибок",
            parameters=self.PGDB_QUEUE_PARAMETERS,
            expressions=list(map(lambda method: ServiceGraphics.DBGraphics.db_method_errors_rps("pgdb", method), self.PGDB_QUEUE_METHODS)),
            extra_data={
                "min": 0,
                "max": 1
            }
        )
        self.update(
            project_id=project_id,
            graphic_id="pgdb_queue_methods_percentile",
            name="Queue: Методы PGDB - время, с (квантиль 99.9%)",
            parameters=self.PGDB_QUEUE_PARAMETERS,
            expressions=list(map(ServiceGraphics.DBGraphics.db_method_percentile, self.PGDB_QUEUE_METHODS)),
            extra_data={
                "min": 0,
                "max": 5,
            }
        )


class TimeMachineDashboards(DashboardBuilder):
    PROJECT = "alice-iot"
    CLUSTER = "timemachine_production|timemachine_beta"
    SERVICE = "timemachine"

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

    CALLBACKS_PARAMETERS = [
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
            "name": "neighbour",
            "value": "*",
        }
    ]

    def __init__(self, oauth_token):
        DashboardBuilder.__init__(self, oauth_token)

    def update_dashboards(self, project_id):
        # self.update_queue_dashboard(project_id)
        # self.update_service_dashboard(project_id)
        # self.update_callbacks_dashboard(project_id)
        self.update_pgdb_queue_dashboard(project_id)

    def update_callbacks_dashboard(self, project_id):
        self.update(
            project_id=project_id,
            dashboard_id="timemachine_callbacks_dashboard",
            name="Timemachine: Callbacks",
            parameters=self.CALLBACKS_PARAMETERS,
            rows=[
                [
                    Panel(
                        title="Callbacks RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "neighbour", "{{neighbour}}",
                            "sensor", "neighbours.calls",
                            "graph", "auto",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                    Panel(
                        title="Callbacks Fails",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "neighbour", "{{neighbour}}",
                            "sensor", "neighbours.fails",
                            "graph", "auto",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                ],
            ])

    def update_pgdb_queue_dashboard(self, project_id):
        self.update(
            project_id=project_id,
            dashboard_id="timemachine_pgdb_queue_dashboard",
            name="Timemachine: PGDB queue stats",
            parameters=self.PARAMETERS,
            rows=[
                [
                    Panel(
                        title="Методы PGDB queue - RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "pgdb_queue_methods_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                    Panel(
                        title="Методы PGDB queue - RPS Ошибок",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "pgdb_queue_methods_errors_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                    Panel(
                        title="Методы PGDB queue - время, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "pgdb_queue_methods_percentile",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                ],
            ]
        )

    def update_queue_dashboard(self, project_id):
        rows = []
        for task in ["TimeMachineHttpCallbackTask"]:
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
            dashboard_id="timemachine_queue_dashboard",
            name="Timemachine: Queue stats",
            parameters=self.PARAMETERS,
            rows=rows,
        )

    def update_service_dashboard(self, project_id):
        self.update(
            project_id=project_id,
            dashboard_id="timemachine_service_dashboard",
            name="Timemachine: Service stats",
            parameters=self.PARAMETERS,
            rows=[
                [
                    Panel(
                        title="Методы API - RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "timemachine_api_handlers_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                    Panel(
                        title="Методы API - RPS HTTP Ошибок",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "timemachine_api_handlers_errors_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                ],
                [
                    Panel(
                        title="Методы API - % HTTP Ошибок",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "timemachine_api_handlers_errors_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                    Panel(
                        title="Методы API - время, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "timemachine_api_handlers_percentiles",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                ],
            ]
        )
