# -*- coding: utf8 -*-
# flake8: noqa
"""
IOT Provider graphics
"""
from collections import namedtuple
from copy import deepcopy

import attr

from alice.iot.dashboards.lib.solomon.dashboard_builder import DashboardBuilder, Panel
from alice.iot.dashboards.lib.solomon.graphic_builder import RGBA
from alice.iot.dashboards.lib.solomon.service_graphics import ServiceGraphics


@attr.s
class NamedMetric(object):
    name = attr.ib()
    color = attr.ib(default=None)
    area = attr.ib(default=True)
    stack = attr.ib(default=None)

    def with_area(self):
        metric = deepcopy(self)
        metric.area = True
        return metric


class ProviderGraphics(ServiceGraphics):
    PROJECT = "paskills-external-data"
    CLUSTER = "iot-prod|iot-beta|iot-dev"
    SERVICE = "smart-home"

    REQUEST_PARAMETERS = [
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
            "name": "skill_id",
            "value": "*",
        },
        {
            "name": "source",
            "value": "*",
        },
        {
            "name": "command",
            "value": "discovery|query|action|unlink|all"
        }
    ]

    CALLBACK_PARAMETERS = [
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
            "name": "skill_id",
            "value": "*",
        },
    ]

    PROTOCOL_PARAMETERS = [
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
            "name": "skill_id",
            "value": "*",
        },
        {
            "name": "source",
            "value": "*",
        }
    ]

    MODEL_MANUFACTURER_PROTOCOL_PARAMETERS = [
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
            "name": "skill_id",
            "value": "*",
        },
        {
            "name": "source",
            "value": "*",
        },
        {
            "name": "model",
            "value": "*",
        },
        {
            "name": "manufacturer",
            "value": "*",
        }
    ]

    request_command_status = (
        NamedMetric("error_http_3xx", RGBA(r=255, g=255, b=82, a=0.5)),
        NamedMetric("error_http_4xx", RGBA(r=255, g=153, b=0, a=0.5)),
        NamedMetric("error_http_5xx", RGBA(r=255, g=0, b=0)),
        NamedMetric("error_other", RGBA(r=255, g=0, b=255, a=0.5)),
        NamedMetric("error_timeout", RGBA(r=100, g=255, b=255, a=0.5)),
        NamedMetric("error_bad_response", RGBA(r=0, g=50, b=100)),
        NamedMetric("ok", RGBA(r=64, g=255, b=64, a=0.3)),
    )

    callback_command_status = (
        NamedMetric("error_other", RGBA(r=255, g=0, b=0)),
        NamedMetric("error_bad_request", RGBA(r=0, g=50, b=100)),
        NamedMetric("ok", RGBA(r=64, g=255, b=64, a=0.3)),
    )

    discovery_command_status = (
        NamedMetric("validation_error", RGBA(r=100, g=100, b=200)),
        NamedMetric("ok", RGBA(r=64, g=255, b=64, a=0.3)),
    )

    query_command_status = (
        NamedMetric("device_busy", RGBA(r=0, g=200, b=255)),
        NamedMetric("device_not_found", RGBA(r=255, g=200, b=0)),
        NamedMetric("device_unreachable", RGBA(r=255, g=255, b=0)),
        NamedMetric("not_supported_in_current_mode", RGBA(r=50, g=0, b=255)),
        NamedMetric("invalid_value", RGBA(r=255, g=0, b=255)),
        NamedMetric("unknown_error", RGBA(r=0, g=0, b=0)),
        NamedMetric("internal_error", RGBA(r=255, g=0, b=0)),
        NamedMetric("ok", RGBA(r=64, g=255, b=64, a=0.3)),
    )

    action_command_status = (
        NamedMetric("device_busy", RGBA(r=0, g=200, b=255)),
        NamedMetric("device_not_found", RGBA(r=255, g=200, b=0)),
        NamedMetric("device_unreachable", RGBA(r=255, g=255, b=0)),
        NamedMetric("not_supported_in_current_mode", RGBA(r=50, g=0, b=255)),
        NamedMetric("invalid_value", RGBA(r=255, g=0, b=255)),
        NamedMetric("unknown_error", RGBA(r=0, g=0, b=0)),
        NamedMetric("internal_error", RGBA(r=255, g=0, b=0)),
        NamedMetric("invalid_action", RGBA(r=255, g=0, b=150)),
        NamedMetric("door_open", RGBA(r=127,g=255,b=212)),
        NamedMetric("lid_open", RGBA(r=102,g=205,b=170)),
        NamedMetric("remote_control_disabled",RGBA(r=64,g=224,b=208)),
        NamedMetric("not_enough_water", RGBA(r=0,g=255,b=255)),
        NamedMetric("low_charge_level", RGBA(r=72,g=209,b=204)),
        NamedMetric("container_full", RGBA(r=0,g=206,b=209)),
        NamedMetric("container_empty", RGBA(r=175,g=238,b=238)),
        NamedMetric("drip_tray_full", RGBA(r=176,g=196,b=222)),
        NamedMetric("device_stuck",RGBA(r=70,g=130,b=180)),
        NamedMetric("device_off",RGBA(r=135,g=206,b=250)),
        NamedMetric("firmware_out_of_date",RGBA(r=135,g=206,b=235)),
        NamedMetric("not_enough_detergent",RGBA(r=0,g=191,b=255)),
        NamedMetric("account_linking_error",RGBA(r=100,g=149,b=237)),
        NamedMetric("human_involvement_needed",RGBA(r=112,g=136,b=153)),
        NamedMetric("ok", RGBA(r=64, g=255, b=64, a=0.3)),
    )

    total_command_status = (
        NamedMetric("device_busy", RGBA(r=0, g=200, b=255)),
        NamedMetric("device_not_found", RGBA(r=255, g=200, b=0)),
        NamedMetric("device_unreachable", RGBA(r=255, g=255, b=0)),
        NamedMetric("not_supported_in_current_mode", RGBA(r=50, g=0, b=255)),
        NamedMetric("invalid_value", RGBA(r=255, g=0, b=255)),
        NamedMetric("unknown_error", RGBA(r=0, g=0, b=0)),
        NamedMetric("internal_error", RGBA(r=255, g=0, b=0)),
        NamedMetric("invalid_action", RGBA(r=255, g=0, b=150)),
        NamedMetric("door_open", RGBA(r=127,g=255,b=212)),
        NamedMetric("lid_open", RGBA(r=102,g=205,b=170)),
        NamedMetric("remote_control_disabled",RGBA(r=64,g=224,b=208)),
        NamedMetric("not_enough_water", RGBA(r=0,g=255,b=255)),
        NamedMetric("low_charge_level", RGBA(r=72,g=209,b=204)),
        NamedMetric("container_full", RGBA(r=0,g=206,b=209)),
        NamedMetric("container_empty", RGBA(r=175,g=238,b=238)),
        NamedMetric("drip_tray_full", RGBA(r=176,g=196,b=222)),
        NamedMetric("device_stuck",RGBA(r=70,g=130,b=180)),
        NamedMetric("device_off",RGBA(r=135,g=206,b=250)),
        NamedMetric("firmware_out_of_date",RGBA(r=135,g=206,b=235)),
        NamedMetric("not_enough_detergent",RGBA(r=0,g=191,b=255)),
        NamedMetric("account_linking_error",RGBA(r=100,g=149,b=237)),
        NamedMetric("human_involvement_needed",RGBA(r=112,g=136,b=153)),
        NamedMetric("ok", RGBA(r=64, g=255, b=64, a=0.3)),
    )

    commands = (
        NamedMetric("discovery", RGBA(r=0, g=255, b=0, a=0.5), area=False),
        NamedMetric("query", RGBA(r=0, g=0, b=255, a=0.5), area=False),
        NamedMetric("action", RGBA(r=255, g=0, b=0, a=0.5), area=False),
        NamedMetric("unlink", RGBA(r=0, g=0, b=0, a=0.5), area=False),
    )

    percentiles = (
        NamedMetric("99", RGBA(r=0, g=127, b=255), area=False),
        NamedMetric("95", RGBA(r=53, g=153, b=255, a=0.6), area=False),
        NamedMetric("90", RGBA(r=102, g=179, b=255, a=0.6), area=False),
    )

    def update_request_graphics(self, project_id):
        self.update(
            project_id=project_id,
            graphic_id="provider_request",
            name="Provider: Вызовы (Кол-во)",
            parameters=self.REQUEST_PARAMETERS,
            expressions=list(map(ServiceGraphics.ProviderGraphics.request, self.request_command_status))
        )
        self.update(
            project_id=project_id,
            graphic_id="provider_request_percent",
            name="Provider: Вызовы (%)",
            parameters=self.REQUEST_PARAMETERS,
            expressions=list(map(ServiceGraphics.ProviderGraphics.request, self.request_command_status)),
            extra_data={
                "normalize": True,
            }
        )
        for period in [5, 30, 120, 1440]:
            self.update(
                project_id=project_id,
                graphic_id="provider_request_{period}m".format(period=period),
                name="Provider: Вызовы за {period} минут (Кол-во)".format(period=period),
                parameters=self.REQUEST_PARAMETERS,
                expressions=list(map(
                    lambda status: ServiceGraphics.ProviderGraphics.request_in_period(status, "{period}m".format(period=period)),
                    self.request_command_status
                ))
            )
            self.update(
                project_id=project_id,
                graphic_id="provider_request_percent_{period}m".format(period=period),
                name="Provider: Вызовы за {period} минут (%)".format(period=period),
                parameters=self.REQUEST_PARAMETERS,
                expressions=list(map(
                    lambda status: ServiceGraphics.ProviderGraphics.request_in_period(status, "{period}m".format(period=period)),
                    self.request_command_status
                )),
                extra_data={
                    "normalize": True,
                }
            )

    def update_request_total_graphics(self, project_id):
        self.update(
            project_id=project_id,
            graphic_id="provider_request_total",
            name="Provider: Вызовы (Всего, Кол-во)",
            parameters=self.PROTOCOL_PARAMETERS,
            expressions=list(map(ServiceGraphics.ProviderGraphics.request_total, self.commands))
        )
        self.update(
            project_id=project_id,
            graphic_id="provider_request_total_http_percent",
            name="Provider: % HTTP Ошибок (%)",
            parameters=self.PROTOCOL_PARAMETERS,
            expressions=list(map(
                ServiceGraphics.ProviderGraphics.request_total_http_percent, self.commands
            ))
        )
        for period in [5, 30, 120, 1440]:
            self.update(
                project_id=project_id,
                graphic_id="provider_request_total_{period}m".format(period=period),
                name="Provider: Вызовы за {period} минут (Всего, Кол-во)".format(period=period),
                parameters=self.PROTOCOL_PARAMETERS,
                expressions=list(map(
                    lambda status: ServiceGraphics.ProviderGraphics.request_total_in_period(status, "{period}m".format(period=period)),
                    self.commands
                ))
            )

    def update_protocol_callback_graphics(self, project_id):
        self.update(
            project_id=project_id,
            graphic_id="provider_discovery_callbacks",
            name="Provider: Вызовы Callback Discovery",
            parameters=self.CALLBACK_PARAMETERS,
            expressions=list(map(
                lambda status: ServiceGraphics.ProviderGraphics.protocol_callback_answers("discovery", status),
                self.callback_command_status
            )),
        )
        self.update(
            project_id=project_id,
            graphic_id="provider_push_discovery_callbacks",
            name="Provider: Вызовы Push Discovery",
            parameters=self.CALLBACK_PARAMETERS,
            expressions=list(map(
                lambda status: ServiceGraphics.ProviderGraphics.protocol_callback_answers("push_discovery", status),
                self.callback_command_status
            )),
        )
        self.update(
            project_id=project_id,
            graphic_id="provider_state_callbacks",
            name="Provider: Вызовы Callback State",
            parameters=self.CALLBACK_PARAMETERS,
            expressions=list(map(
                lambda status: ServiceGraphics.ProviderGraphics.protocol_callback_answers("state", status),
                self.callback_command_status
            )),
        )
        self.update(
            project_id=project_id,
            graphic_id="provider_callbacks_per_skill_id",
            name="Provider: Callback RPS per skill_id",
            parameters=[
                {
                    "name": "project",
                    "value": self.PROJECT,
                },
                {
                    "name": "cluster",
                    "value": self.CLUSTER,
                },
                {
                    "name": "service",
                    "value": self.SERVICE,
                },
                {
                    "name": "host",
                    "value": "*",
                }
            ],
            expressions=ServiceGraphics.ProviderGraphics.protocol_callbacks_per_skill_id()
        )

    def update_discovery_graphics(self, project_id):
        self.update(
            project_id=project_id,
            graphic_id="provider_discovery",
            name="Provider: Вызовы Discovery (Кол-во)",
            parameters=self.PROTOCOL_PARAMETERS,
            expressions=list(map(
                lambda status: ServiceGraphics.ProviderGraphics.protocol_answers("discovery", status), self.discovery_command_status
            )),
        )
        self.update(
            project_id=project_id,
            graphic_id="provider_discovery_percent",
            name="Provider: Вызовы Discovery (%)",
            parameters=self.PROTOCOL_PARAMETERS,
            expressions=list(map(
                lambda status: ServiceGraphics.ProviderGraphics.protocol_answers("discovery", status),
                self.discovery_command_status
            )),
            extra_data={
                "normalize": True,
            }
        )
        for period in [5, 30, 120, 1440]:
            self.update(
                project_id=project_id,
                graphic_id="provider_discovery_{period}m".format(period=period),
                name="Provider: Вызовы Discovery за {period} минут (Кол-во)".format(period=period),
                parameters=self.PROTOCOL_PARAMETERS,
                expressions=list(map(
                    lambda status: ServiceGraphics.ProviderGraphics.protocol_answers_in_period("discovery", status,
                                                                               "{period}m".format(period=period)),
                    self.discovery_command_status
                )),
            )
            self.update(
                project_id=project_id,
                graphic_id="provider_discovery_percent_{period}m".format(period=period),
                name="Provider: Вызовы Discovery за {period} минут (%)".format(period=period),
                parameters=self.PROTOCOL_PARAMETERS,
                expressions=list(map(
                    lambda status: ServiceGraphics.ProviderGraphics.protocol_answers_in_period("discovery", status,
                                                                               "{period}m".format(period=period)),
                    self.discovery_command_status
                )),
                extra_data={
                    "normalize": True,
                }
            )

    def update_query_graphics(self, project_id):
        self.update(
            project_id=project_id,
            graphic_id="provider_query",
            name="Provider: Вызовы Query (Кол-во)",
            parameters=self.PROTOCOL_PARAMETERS,
            expressions=list(map(
                lambda status: ServiceGraphics.ProviderGraphics.protocol_answers("query", status), self.query_command_status
            )),
        )
        self.update(
            project_id=project_id,
            graphic_id="provider_query_percent",
            name="Provider: Вызовы Query (%)",
            parameters=self.PROTOCOL_PARAMETERS,
            expressions=list(map(
                lambda status: ServiceGraphics.ProviderGraphics.protocol_answers("query", status), self.query_command_status
            )),
            extra_data={
                "normalize": True,
            }
        )
        for period in [5, 30, 120, 1440]:
            self.update(
                project_id=project_id,
                graphic_id="provider_query_{period}m".format(period=period),
                name="Provider: Вызовы Query за {period} минут (Кол-во)".format(period=period),
                parameters=self.PROTOCOL_PARAMETERS,
                expressions=list(map(
                    lambda status: ServiceGraphics.ProviderGraphics.protocol_answers_in_period("query", status,
                                                                               "{period}m".format(period=period)),
                    self.query_command_status
                )),
            )
            self.update(
                project_id=project_id,
                graphic_id="provider_query_percent_{period}m".format(period=period),
                name="Provider: Вызовы Query за {period} минут (%)".format(period=period),
                parameters=self.PROTOCOL_PARAMETERS,
                expressions=list(map(
                    lambda status: ServiceGraphics.ProviderGraphics.protocol_answers_in_period("query", status,
                                                                               "{period}m".format(period=period)),
                    self.query_command_status
                )),
                extra_data={
                    "normalize": True,
                }
            )

    def update_action_graphics(self, project_id):
        self.update(
            project_id=project_id,
            graphic_id="provider_action",
            name="Provider: Вызовы Action (Кол-во)",
            parameters=self.PROTOCOL_PARAMETERS,
            expressions=list(map(
                lambda status: ServiceGraphics.ProviderGraphics.protocol_answers("action", status), self.action_command_status
            )),
        )
        self.update(
            project_id=project_id,
            graphic_id="provider_action_percent",
            name="Provider: Вызовы Action (%)",
            parameters=self.PROTOCOL_PARAMETERS,
            expressions=list(map(
                lambda status: ServiceGraphics.ProviderGraphics.protocol_answers("action", status), self.action_command_status
            )),
            extra_data={
                "normalize": True,
            }
        )
        for period in [5, 30, 120, 1440]:
            self.update(
                project_id=project_id,
                graphic_id="provider_action_{period}m".format(period=period),
                name="Provider: Вызовы Action за {period} минут (Кол-во)".format(period=period),
                parameters=self.PROTOCOL_PARAMETERS,
                expressions=list(map(
                    lambda status: ServiceGraphics.ProviderGraphics.protocol_answers_in_period("action", status,
                                                                               "{period}m".format(period=period)),
                    self.action_command_status
                )),
            )
            self.update(
                project_id=project_id,
                graphic_id="provider_action_percent_{period}m".format(period=period),
                name="Provider: Вызовы Action за {period} минут (%)".format(period=period),
                parameters=self.PROTOCOL_PARAMETERS,
                expressions=list(map(
                    lambda status: ServiceGraphics.ProviderGraphics.protocol_answers_in_period("action", status,
                                                                               "{period}m".format(period=period)),
                    self.action_command_status
                )),
                extra_data={
                    "normalize": True,
                }
            )

    def update_protocol_answers_total_graphics(self, project_id):
        self.update(
            project_id=project_id,
            graphic_id="provider_protocol_answers_total",
            name="Provider: Ответы по протоколу (Всего, Кол-во)",
            parameters=self.PROTOCOL_PARAMETERS,
            expressions=list(map(
                lambda status: ServiceGraphics.ProviderGraphics.protocol_answers("all", status), self.total_command_status
            )),
        )
        self.update(
            project_id=project_id,
            graphic_id="provider_protocol_answers_total_percent",
            name="Provider: Ответы по протоколу (Всего, %)",
            parameters=self.PROTOCOL_PARAMETERS,
            expressions=list(map(
                lambda status: ServiceGraphics.ProviderGraphics.protocol_answers("all", status), self.total_command_status
            )),
            extra_data={
                "normalize": True,
            }
        )
        for period in [5, 30, 120, 1440]:
            self.update(
                project_id=project_id,
                graphic_id="provider_protocol_answers_total_{period}m".format(period=period),
                name="Provider: Ответы по протоколу {period} минут (Всего, Кол-во)".format(period=period),
                parameters=self.PROTOCOL_PARAMETERS,
                expressions=list(map(
                    lambda status: ServiceGraphics.ProviderGraphics.protocol_answers_in_period("all", status,
                                                                               "{period}m".format(period=period)),
                    self.total_command_status
                )),
            )
            self.update(
                project_id=project_id,
                graphic_id="provider_protocol_answers_total_percent_{period}m".format(period=period),
                name="Provider: Ответы по протоколу {period} минут (Всего, %)".format(period=period),
                parameters=self.PROTOCOL_PARAMETERS,
                expressions=list(map(
                    lambda status: ServiceGraphics.ProviderGraphics.protocol_answers_in_period("all", status,
                                                                               "{period}m".format(period=period)),
                    self.total_command_status
                )),
                extra_data={
                    "normalize": True,
                }
            )

    def update_protocol_answers_graphics(self, project_id):
        self.update_discovery_graphics(project_id)
        self.update_query_graphics(project_id)
        self.update_action_graphics(project_id)

    def update_model_manufacturer_discovery_graphics(self, project_id):
        self.update(
            project_id=project_id,
            graphic_id="provider_discovery_model_manufacturer",
            name="Provider: Вызовы Discovery по модели и производителю (Кол-во)",
            parameters=self.MODEL_MANUFACTURER_PROTOCOL_PARAMETERS,
            expressions=list(map(
                lambda status: ServiceGraphics.ProviderGraphics.protocol_answers_model_manufacturer("discovery", status),
                self.discovery_command_status
            )),
        )
        self.update(
            project_id=project_id,
            graphic_id="provider_discovery_model_manufacturer_percent",
            name="Provider: Вызовы Discovery по модели и производителю (%)",
            parameters=self.MODEL_MANUFACTURER_PROTOCOL_PARAMETERS,
            expressions=list(map(
                lambda status: ServiceGraphics.ProviderGraphics.protocol_answers_model_manufacturer("discovery", status),
                self.discovery_command_status
            )),
            extra_data={
                "normalize": True,
            }
        )

    def update_model_manufacturer_query_graphics(self, project_id):
        self.update(
            project_id=project_id,
            graphic_id="provider_query_model_manufacturer",
            name="Provider: Вызовы Query по модели и производителю (Кол-во)",
            parameters=self.MODEL_MANUFACTURER_PROTOCOL_PARAMETERS,
            expressions=list(map(
                lambda status: ServiceGraphics.ProviderGraphics.protocol_answers_model_manufacturer("query", status),
                self.query_command_status
            )),
        )
        self.update(
            project_id=project_id,
            graphic_id="provider_query_model_manufacturer_percent",
            name="Provider: Вызовы Query по модели и производителю (%)",
            parameters=self.MODEL_MANUFACTURER_PROTOCOL_PARAMETERS,
            expressions=list(map(
                lambda status: ServiceGraphics.ProviderGraphics.protocol_answers_model_manufacturer("query", status),
                self.query_command_status
            )),
            extra_data={
                "normalize": True,
            }
        )

    def update_model_manufacturer_action_graphics(self, project_id):
        self.update(
            project_id=project_id,
            graphic_id="provider_action_model_manufacturer",
            name="Provider: Вызовы Action по модели и производителю (Кол-во)",
            parameters=self.MODEL_MANUFACTURER_PROTOCOL_PARAMETERS,
            expressions=list(map(
                lambda status: ServiceGraphics.ProviderGraphics.protocol_answers_model_manufacturer("action", status),
                self.action_command_status
            )),
        )
        self.update(
            project_id=project_id,
            graphic_id="provider_action_model_manufacturer_percent",
            name="Provider: Вызовы Action по модели и производителю (%)",
            parameters=self.MODEL_MANUFACTURER_PROTOCOL_PARAMETERS,
            expressions=list(map(
                lambda status: ServiceGraphics.ProviderGraphics.protocol_answers_model_manufacturer("action", status),
                self.action_command_status
            )),
            extra_data={
                "normalize": True,
            }
        )

    def update_model_manufacturer_protocol_answers_total_graphics(self, project_id):
        self.update(
            project_id=project_id,
            graphic_id="provider_protocol_model_manufacturer_answers_total",
            name="Provider: Ответы по протоколу (Всего, Кол-во)",
            parameters=self.MODEL_MANUFACTURER_PROTOCOL_PARAMETERS,
            expressions=list(map(
                lambda status: ServiceGraphics.ProviderGraphics.protocol_answers_model_manufacturer("all", status),
                self.total_command_status
            )),
        )
        self.update(
            project_id=project_id,
            graphic_id="provider_protocol_modman_answers_total_percent",
            name="Provider: Ответы по протоколу (Всего, %)",
            parameters=self.MODEL_MANUFACTURER_PROTOCOL_PARAMETERS,
            expressions=list(map(
                lambda status: ServiceGraphics.ProviderGraphics.protocol_answers_model_manufacturer("all", status),
                self.total_command_status
            )),
            extra_data={
                "normalize": True,
            }
        )

    def update_model_manufacturer_protocol_answers_graphics(self, project_id):
        self.update_model_manufacturer_discovery_graphics(project_id)
        self.update_model_manufacturer_query_graphics(project_id)
        self.update_model_manufacturer_action_graphics(project_id)

    def update_graphics(self, project_id):
        # self.update_request_graphics(project_id)
        # self.update_request_total_graphics(project_id)
        # self.update_protocol_answers_graphics(project_id)
        self.update_protocol_callback_graphics(project_id)
        # self.update_protocol_answers_total_graphics(project_id)
        # self.update_model_manufacturer_protocol_answers_graphics(project_id)
        # self.update_model_manufacturer_protocol_answers_total_graphics(project_id)

        # self.update(
        #     project_id=project_id,
        #     graphic_id="provider_command_percentiles",
        #     name="Provider: Тайминги, с",
        #     parameters=self.REQUEST_PARAMETERS,
        #     expressions=list(map(ServiceGraphics.ProviderGraphics.percentiles_command, self.percentiles)),
        #     extra_data={
        #         "interpolate": "RIGHT",
        #     },
        # )
        # self.update(
        #     project_id=project_id,
        #     graphic_id="provider_percentiles",
        #     name="Provider: Тайминги, с (квантиль 99.9%)",
        #     parameters=self.PROTOCOL_PARAMETERS,
        #     expressions=list(map(ServiceGraphics.ProviderGraphics.percentile, self.commands)),
        #     extra_data={
        #         "interpolate": "RIGHT",
        #     },
        # )


Provider = namedtuple('Provider', ['name', 'skill_id'])


class ProviderDashboard(DashboardBuilder):
    PROJECT = "paskills-external-data"
    CLUSTER = "iot-prod|iot-beta"
    SERVICE = "smart-home"

    PROVIDER_PARAMETERS = [
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
            "name": "source",
            "value": "*",
        },
        {
            "name": "skill_id",
            "value": "*",
        }
    ]

    MODEL_MANUFACTURER_PROTOCOL_PARAMETERS = [
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
            "name": "skill_id",
            "value": "*",
        },
        {
            "name": "source",
            "value": "*",
        },
        {
            "name": "model",
            "value": "*",
        },
        {
            "name": "manufacturer",
            "value": "*",
        }
    ]

    def __init__(self, oauth_token):
        DashboardBuilder.__init__(self, oauth_token)

    def update_dashboards(self, project_id):
        self.update_provider_dashboard(project_id)

    def update_provider_dashboard(self, project_id):
        self.update(
            project_id=project_id,
            dashboard_id="provider_dashboard",
            name="Provider: Stats",
            parameters=self.PROVIDER_PARAMETERS,
            rows=[
                [
                    Panel(
                        title="Вызовы, Кол-во",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "source", "{{source}}",
                            "graph", "provider_request_total",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="HTTP Ошибки, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "source", "{{source}}",
                            "graph", "provider_request_total_http_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
                [
                    Panel(
                        title="Ответы по протоколу, Кол-во",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "source", "{{source}}",
                            "graph", "provider_protocol_answers_total",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Ответы по протоколу, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "source", "{{source}}",
                            "graph", "provider_protocol_answers_total_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Тайминги, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "source", "{{source}}",
                            "graph", "provider_percentiles",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ],
                [
                    Panel(
                        title="Ответы Discovery, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "source", "{{source}}",
                            "graph", "provider_discovery_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Ответы Query, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "source", "{{source}}",
                            "graph", "provider_query_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Ответы Action, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "source", "{{source}}",
                            "graph", "provider_action_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    )
                ],
                [
                    Panel(
                        title="Callback Discovery",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "graph", "provider_discovery_callbacks",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Push Discovery",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "graph", "provider_push_discovery_callbacks",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Callback State",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "graph", "provider_state_callbacks",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ],
            ]
        )
        self.update(
            project_id=project_id,
            dashboard_id="provider_http_dashboard",
            name="Provider: HTTP Stats",
            parameters=self.PROVIDER_PARAMETERS,
            rows=[
                [
                    Panel(
                        title="Вызовы, Кол-во",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "source", "{{source}}",
                            "graph", "provider_request_total",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Тайминги, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "source", "{{source}}",
                            "graph", "provider_percentiles",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
                [
                    Panel(
                        title="Вызовы Discovery, Кол-во",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "source", "{{source}}",
                            "command", "discovery",
                            "graph", "provider_request",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Вызовы Discovery, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "source", "{{source}}",
                            "command", "discovery",
                            "graph", "provider_request",
                            "norm", "true",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Тайминги Discovery, с (квантили)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "source", "{{source}}",
                            "command", "discovery",
                            "graph", "provider_command_percentiles",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ],
                [
                    Panel(
                        title="Вызовы Query, Кол-во",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "source", "{{source}}",
                            "command", "query",
                            "graph", "provider_request",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Вызовы Query, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "source", "{{source}}",
                            "command", "query",
                            "graph", "provider_request",
                            "norm", "true",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Тайминги Query, с (квантили)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "source", "{{source}}",
                            "command", "query",
                            "graph", "provider_command_percentiles",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ],
                [
                    Panel(
                        title="Вызовы Action, Кол-во",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "source", "{{source}}",
                            "command", "action",
                            "graph", "provider_request",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Вызовы Action, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "source", "{{source}}",
                            "command", "action",
                            "graph", "provider_request",
                            "norm", "true",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Тайминги Action, с (квантили)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "source", "{{source}}",
                            "command", "action",
                            "graph", "provider_command_percentiles",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ],
                [
                    Panel(
                        title="Вызовы Unlink, Кол-во",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "source", "{{source}}",
                            "command", "unlink",
                            "graph", "provider_request",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Вызовы Unlink, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "source", "{{source}}",
                            "command", "unlink",
                            "graph", "provider_request",
                            "norm", "true",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Тайминги Unlink, с (квантили)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "source", "{{source}}",
                            "command", "unlink",
                            "graph", "provider_command_percentiles",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ],
            ]
        )
        self.update(
            project_id=project_id,
            dashboard_id="provider_model_manufacturer_dashboard",
            name="Provider: Model/Manufacturer Stats",
            parameters=self.MODEL_MANUFACTURER_PROTOCOL_PARAMETERS,
            rows=[
                [
                    Panel(
                        title="Ответы по протоколу, Кол-во",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "source", "{{source}}",
                            "model", "{{model}}",
                            "manufacturer", "{{manufacturer}}",
                            "graph", "provider_protocol_model_manufacturer_answers_total",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Ответы по протоколу, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "source", "{{source}}",
                            "model", "{{model}}",
                            "manufacturer", "{{manufacturer}}",
                            "graph", "provider_protocol_modman_answers_total_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
                [
                    Panel(
                        title="Ответы Discovery, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "source", "{{source}}",
                            "model", "{{model}}",
                            "manufacturer", "{{manufacturer}}",
                            "graph", "provider_discovery_model_manufacturer_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Ответы Query, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "source", "{{source}}",
                            "model", "{{model}}",
                            "manufacturer", "{{manufacturer}}",
                            "graph", "provider_query_model_manufacturer_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Ответы Action, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "source", "{{source}}",
                            "model", "{{model}}",
                            "manufacturer", "{{manufacturer}}",
                            "graph", "provider_action_model_manufacturer_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    )
                ],
            ]
        )
