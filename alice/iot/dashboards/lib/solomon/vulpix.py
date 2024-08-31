# -*- coding: utf8 -*-
# flake8: noqa
"""
Vulpix graphics
"""

from alice.iot.dashboards.lib.solomon.aql import AQL
from alice.iot.dashboards.lib.solomon.dashboard_builder import DashboardBuilder, Panel
from alice.iot.dashboards.lib.solomon.graphic_builder import Expression
from alice.iot.dashboards.lib.solomon.graphic_builder import RGBA
from alice.iot.dashboards.lib.solomon.provider import NamedMetric
from alice.iot.dashboards.lib.solomon.service_graphics import ServiceGraphics, NamedProcessor


class VulpixGraphics(ServiceGraphics):
    PROJECT = "alice-iot"
    CLUSTER = "vulpix_production|vulpix_beta"
    SERVICE = "vulpix"

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

    processors = [
        NamedProcessor("connect", "1 шаг подключения"),
        NamedProcessor("connect_2", "2 шаг подключения"),
        NamedProcessor("success", "Успешное подключение"),
        NamedProcessor("failure_run", "Не нашлось на каком-то из шагов подключения"),
        NamedProcessor("failure_apply_1", "Не нашлось на 1-ом шаге подключения"),
        NamedProcessor("failure_apply_2", "Не нашлось на 2-ом шаге подключения"),
        NamedProcessor("start_run", "Начало подключения"),
        NamedProcessor("start_apply", "Начало 1-ого подключения"),
        NamedProcessor("start_apply_2", "Начало 2-ого подключения"),
        NamedProcessor("cancel", "Отмена подключения"),

        # v2
        NamedProcessor("failure_run_v2", "Не нашлось на каком-то из шагов подключения (v2)"),
        NamedProcessor("failure_apply_1_v2", "Не нашлось на 1-ом шаге подключения (v2)"),
        NamedProcessor("failure_apply_2_v2", "Не нашлось на 2-ом шаге подключения (v2)"),
        NamedProcessor("start_run_v2", "Начало подключения (v2)"),
        NamedProcessor("start_apply_v2", "Начало 1-ого подключения (v2)"),
        NamedProcessor("start_apply_2_v2", "Начало 2-ого подключения (v2)"),
        NamedProcessor("success_v2", "Успешное подключение (v2)"),
    ]

    # failureRunV2WithMetrics := NewRunProcessorWithMetrics(failureRunV2, metrics, "failure_run_v2")
    # failureStep1ApplyV2WithMetrics := NewApplyProcessorWithMetrics(failureStep1ApplyV2, metrics, "failure_apply_1_v2")
    # failureStep2ApplyV2WithMetrics := NewApplyProcessorWithMetrics(failureStep2ApplyV2, metrics, "failure_apply_2_v2")
    # startRunV2WithMetrics := NewRunProcessorWithMetrics(startRunV2, metrics, "start_run_v2")
    # startStep1V2WithMetrics := NewContinueProcessorWithMetrics(startStep1V2, metrics, "start_apply_v2")
    # startStep2V2WithMetrics := NewContinueProcessorWithMetrics(startStep2V2, metrics, "start_apply_2_v2")
    # successV2WithMetrics := NewRunProcessorWithMetrics(successV2, metrics, "success_v2")


    commands = [
        NamedMetric(name="ok", color=RGBA(r=10, g=255, b=10, a=0.4), stack=True, area=True),
        NamedMetric(name="unknown_error", color=RGBA(r=200, g=0, b=0, a=0.8), stack=True, area=True)
    ]

    def __init__(self, oauth_token):
        ServiceGraphics.__init__(self, oauth_token)

    @staticmethod
    def shift_MSK_to_UTC(expression):
        return AQL.shift(expression, "-180m")

    @staticmethod
    def sum_in_period(expression, period):
        preaggregate_expression = AQL.diff_expression(AQL.integrate_expression(AQL.group_lines("sum", expression)))
        return AQL.aggregate_expression("sum", preaggregate_expression, period)

    @staticmethod
    def processor_requests(processor_name, step, command_status):
        return '''
        {{sensor="processor.requests",processor="{processor}",step="{step}",command_status="{command_status}"}}
        '''.format(processor=processor_name, step=step, command_status=command_status)

    def processor_step_command_status(self, named_processor, command_status, step):
        expression = self.processor_requests(named_processor.code_name, step, command_status.name)
        return Expression(
            expression=self.shift_MSK_to_UTC(self.sum_in_period(expression, "1440m")),
            title="{processor}".format(processor=named_processor.view_name),
            color=command_status.color,
            area=command_status.area,
            stack=command_status.stack,
        )

    @staticmethod
    def connect_client_requests(client):
        return '''
        {{sensor="connect.per_client_requests",client_info="{client}"}}
        '''.format(client=client)

    def connect_client(self, client, named_metric):
        expression = self.connect_client_requests(client)
        return Expression(
            expression=self.shift_MSK_to_UTC(self.sum_in_period(expression, "1440m")),
            title="{metric_name}".format(metric_name=named_metric.name),
            color=named_metric.color,
            area=named_metric.area,
            stack=named_metric.stack,
        )

    def first_step_success(self):
        first_step_failure_v1 = self.processor_requests("failure_apply_1", "apply", "ok")
        first_step_apply_v1 = self.processor_requests("start_apply", "apply", "ok")
        first_step_success_v1 = AQL.sub(first_step_apply_v1, first_step_failure_v1)

        first_step_failure_v2 = self.processor_requests("failure_apply_1_v2", "apply", "ok")
        first_step_apply_v2 = self.processor_requests("start_apply_v2", "continue", "ok")
        first_step_success_v2 = AQL.sub(first_step_apply_v2, first_step_failure_v2)

        first_step_success = AQL.sum(first_step_success_v1, first_step_success_v2)
        return Expression(
            expression=self.shift_MSK_to_UTC(self.sum_in_period(first_step_success, "1440m")),
            title="Успешное подключение на 1 шаге",
            color=RGBA(r=10, g=255, b=10, a=0.4),
            area=True,
            stack=True,
        )

    def first_step_failure(self):
        first_step_failure_v1 = self.processor_requests("failure_apply_1", "apply", "ok")
        first_step_failure_v2 = self.processor_requests("failure_apply_1_v2", "apply", "ok")
        first_step_failure = AQL.sum(first_step_failure_v1, first_step_failure_v2)
        return Expression(
            expression=self.shift_MSK_to_UTC(self.sum_in_period(first_step_failure, "1440m")),
            title="Не нашлось на первом шаге подключения",
            color=RGBA(r=249, g=166, b=2, a=0.8),
            area=True,
            stack=True,
        )

    def second_step_success(self):
        second_step_failure_v1 = self.processor_requests("failure_apply_2", "apply", "ok")
        second_step_apply_v1 = self.processor_requests("start_apply_2", "apply", "ok")
        second_step_success_v1 = AQL.sub(second_step_apply_v1, second_step_failure_v1)

        second_step_failure_v2 = self.processor_requests("failure_apply_2_v2", "apply", "ok")
        second_step_apply_v2 = self.processor_requests("start_apply_v2", "continue", "ok")
        second_step_success_v2 = AQL.sub(second_step_apply_v2, second_step_failure_v2)

        second_step_success = AQL.sum(second_step_success_v1, second_step_success_v2)
        return Expression(
            expression=self.shift_MSK_to_UTC(self.sum_in_period(second_step_success, "1440m")),
            title="Успешное подключение на 2 шаге",
            color=RGBA(r=11, g=102, b=35, a=1),
            area=True,
            stack=True,
        )

    def second_step_failure(self):
        second_step_failure_v1 = self.processor_requests("failure_apply_2", "apply", "ok")

        second_step_failure_v2 = self.processor_requests("failure_apply_2_v2", "apply", "ok")

        second_step_failure = AQL.sum(second_step_failure_v1, second_step_failure_v2)
        return Expression(
            expression=self.shift_MSK_to_UTC(self.sum_in_period(second_step_failure, "1440m")),
            title="Не нашлось на втором шаге подключения",
            color=RGBA(r=200, g=0, b=0, a=0.8),
            area=True,
            stack=True,
        )

    def all_failure(self):
        first_step_failure_v1 = self.processor_requests("failure_apply_1", "apply", "ok")
        second_step_start_v1 = self.processor_requests("start_apply_2", "apply", "ok")
        second_step_failure_v1 = self.processor_requests("failure_apply_2", "apply", "ok")
        all_failure_v1 = AQL.sum(AQL.sub(first_step_failure_v1, second_step_start_v1), second_step_failure_v1)

        first_step_failure_v2 = self.processor_requests("failure_apply_1_v2", "apply", "ok")
        second_step_start_v2 = self.processor_requests("start_apply_2_v2", "continue", "ok")
        second_step_failure_v2 = self.processor_requests("failure_apply_2_v2", "apply", "ok")
        all_failure_v2 = AQL.sum(AQL.sub(first_step_failure_v2, second_step_start_v2), second_step_failure_v2)

        all_failure = AQL.sum(all_failure_v1, all_failure_v2)

        return Expression(
            expression=self.shift_MSK_to_UTC(self.sum_in_period(all_failure, "1440m")),
            title="Не нашлось устройство",
            color=RGBA(r=200, g=0, b=0, a=0.8),
            area=True,
            stack=True,
        )

    def connect_scenario_entrance(self):
        return self.processor_step_command_status(
            NamedProcessor("connect", "Запуск сценария"),
            NamedMetric(name="ok", color=RGBA(r=144, g=144, b=144, a=0.5), area=True),
            "apply",
        )

    def update_processor_graphics(self, project_id):
        self.update(
            project_id=project_id,
            graphic_id="vulpix_processor_request_errors",
            name="Vulpix: Processor RPS ошибок",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.ProcessorGraphics.errors_rps, self.processors)),
            extra_data={
                "min": 0,
                "max": 5,
            }
        )
        self.update(
            project_id=project_id,
            graphic_id="vulpix_processor_request_total",
            name="Vulpix: Processor Total RPS",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.ProcessorGraphics.processor_rps, self.processors))
        )
        self.update(
            project_id=project_id,
            graphic_id="vulpix_processor_percentiles",
            name="Vulpix: Processor Timings, с (квантиль 99.9%)",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.ProcessorGraphics.processor_percentile, self.processors))
        )

    def update_pairing_graphics(self, project_id):
        self.update(
            project_id=project_id,
            graphic_id="vulpix_pairing_total",
            name="Vulpix: Количество подключений",
            parameters=self.PARAMETERS,
            expressions=[
                self.all_failure(),
                self.connect_scenario_entrance(),
                self.connect_client(
                    "unsupported_client",
                    NamedMetric(name="Неподдерживаемый клиент", color=RGBA(r=156, g=0, b=255, a=0.8), area=True, stack=True),
                ),
                self.connect_client(
                    "unsupported_speaker",
                    NamedMetric(name="Неподдерживаемая колонка", color=RGBA(r=200, g=140, b=255, a=0.8), area=True, stack=True),
                ),
                self.connect_client(
                    "search_app",
                    NamedMetric(name="ПП", color=RGBA(r=0, g=120, b=120, a=0.8), area=True, stack=True),
                ),
                self.connect_client(
                    "unsupported_network",
                    NamedMetric(name="Неподдерживаемая сеть", color=RGBA(r=0, g=60, b=60, a=0.8), area=True, stack=True),
                ),
                self.second_step_success(),
                self.first_step_success(),
            ],
            extra_data={
                "interpolate": "LEFT",
            })
        self.update(
            project_id=project_id,
            graphic_id="vulpix_pairing_total_step_1",
            name="Vulpix: Количество подключений на первом шаге",
            parameters=self.PARAMETERS,
            expressions=[
                self.first_step_failure(),
                self.first_step_success(),
            ],
            extra_data={
                "interpolate": "LEFT",
            })
        self.update(
            project_id=project_id,
            graphic_id="vulpix_pairing_total_step_2",
            name="Vulpix: Количество подключений на втором шаге",
            parameters=self.PARAMETERS,
            expressions=[
                self.second_step_failure(),
                self.second_step_success(),
            ],
            extra_data={
                "interpolate": "LEFT",
            })

    def update_graphics(self, project_id):
        self.update_processor_graphics(project_id)
        self.update_pairing_graphics(project_id)


class VulpixDashboards(DashboardBuilder):
    PROJECT = "alice-iot"
    CLUSTER = "vulpix_production|vulpix_beta"
    SERVICE = "vulpix"

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

    def __init__(self, oauth_token):
        DashboardBuilder.__init__(self, oauth_token)

    def update_dashboards(self, project_id):
        self.update_processors_dashboard(project_id)
        self.update_pairing_dashboard(project_id)

    def update_processors_dashboard(self, project_id):
        self.update(
            project_id=project_id,
            dashboard_id="vulpix_processors_dashboard",
            name="Vulpix: Processors stats",
            parameters=self.PARAMETERS,
            rows=[
                [
                    Panel(
                        title="Processors: Run Total RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "step", "run",
                            "graph", "vulpix_processor_request_total",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Processors: Run Errors RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "vulpix_processor_request_errors",
                            "step", "run",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Processors: Run Timings, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "vulpix_processor_percentiles",
                            "step", "run",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ],
                [
                    Panel(
                        title="Processors: Apply Total RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "step", "apply",
                            "graph", "vulpix_processor_request_total",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Processors: Apply Errors RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "vulpix_processor_request_errors",
                            "step", "apply",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Processors: Apply Timings, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "vulpix_processor_percentiles",
                            "step", "apply",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ],
            ])

    def update_pairing_dashboard(self, project_id):
        self.update(
            project_id=project_id,
            dashboard_id="vulpix_pairing_dashboard",
            name="Vulpix: Pairing stats",
            parameters=self.PARAMETERS,
            rows=[
                [
                    Panel(
                        title="Pairing: Total stats",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "vulpix_pairing_total",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ],
                [
                    Panel(
                        title="Pairing: Total Step 1 stats",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "vulpix_pairing_total_step_1",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ],
                [
                    Panel(
                        title="Pairing: Total Step 2 Stats",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "vulpix_pairing_total_step_2",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ],
            ])
