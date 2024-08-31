# -*- coding: utf8 -*-
# flake8: noqa
"""
Tuya graphics
"""

from alice.iot.dashboards.lib.solomon.dashboard_builder import DashboardBuilder, Panel
from alice.iot.dashboards.lib.solomon.graphic_builder import RGBA
from alice.iot.dashboards.lib.solomon.provider import NamedMetric
from alice.iot.dashboards.lib.solomon.service_graphics import ServiceGraphics, HttpHandler


class TuyaGraphics(ServiceGraphics):
    PROJECT = "alice-iot"
    CLUSTER = "tuya_producton|tuya_beta"
    SERVICE = "tuya"

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
    BY_DC_PARAMETERS = [
        {
            "name": "project",
            "value": PROJECT,
        },
        {
            "name": "cluster",
            "value": "tuya_production",
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

    UI_HANDLERS = [
        HttpHandler(path="/m/tokens/", method="POST"),
        HttpHandler(path="/m/tokens/{token}/devices", method="PUT"),
        HttpHandler(path="/m/ir/{device_id}/categories/", method="GET"),
        HttpHandler(path="/m/ir/{device_id}/categories/{category_id}/brands", method="GET"),
        HttpHandler(path="/m/ir/{device_id}/categories/{category_id}/brands/{brand_id}/presets", method="GET"),
        HttpHandler(path="/m/ir/{device_id}/categories/{category_id}/brands/{brand_id}/presets/{preset_id}/control",
                    method="GET"),
        HttpHandler(path="/m/ir/{device_id}/command", method="POST"),
        HttpHandler(path="/m/ir/{device_id}/control", method="POST"),

    ]

    TUYA_API_CALLS = [
        NamedMetric(name="create_user"),
        NamedMetric(name="get_token"),
        NamedMetric(name="get_pairing_token"),
        NamedMetric(name="get_cipher"),
        NamedMetric(name="get_devices_under_pairing_token"),
        NamedMetric(name="get_device_by_id"),
        NamedMetric(name="get_device_firmware_info"),
        NamedMetric(name="get_ac_status"),
        NamedMetric(name="delete_device"),
        NamedMetric(name="delete_ir_control"),
        NamedMetric(name="get_user_devices"),
        NamedMetric(name="send_commands_to_device"),
        NamedMetric(name="get_ir_categories"),
        NamedMetric(name="get_ir_category_brands"),
        NamedMetric(name="get_ir_category_brand_presets"),
        NamedMetric(name="get_ir_category_brand_presets_keys_map"),
        NamedMetric(name="send_ir_command"),
        NamedMetric(name="send_ir_ac_command"),
        NamedMetric(name="add_ir_remote_control"),
        NamedMetric(name="get_ir_remotes_for_transmitter"),
        NamedMetric(name="send_ir_ac_power_command"),
    ]

    BLACKBOX_API_CALLS = [
        NamedMetric(name="oauth"),
        NamedMetric(name="sessionID"),
    ]

    TVM_API_CALLS = [
        NamedMetric(name="getServiceTicket"),
        NamedMetric(name="checkServiceTicket"),
        NamedMetric(name="checkUserTicket"),
    ]

    CODES = {
        "4xx": NamedMetric(name="4xx", color=RGBA(r=255, g=153, b=0, a=0.5)),
        "5xx": NamedMetric(name="5xx", color=RGBA(r=255, g=0, b=0)),
        "fails": NamedMetric(name="unavailable", color=RGBA(r=100, g=50, b=0)),
        "2xx": NamedMetric(name="2xx", color=RGBA(r=64, g=255, b=64, a=0.3)),
    }

    YDB_METHODS = [
        "createUser",
        "getTuyaUser",
        "isKnownUser",
        "getDeviceOwner",
        "setDevicesOwner",
        "invalidateDeviceOwner",
    ]

    def __init__(self, oauth_token):
        ServiceGraphics.__init__(self, oauth_token)

    def update_graphics(self, project_id):
        # self.update_ui_graphics(project_id)
        # self.update_ydb_graphics(project_id)
        self.update_api_graphics(project_id)
        # self.update_perf_graphics(project_id)
        # self.update_neighbour_graphics(project_id)
        # self.update_by_dc_graphics(project_id)

    def update_ui_graphics(self, project_id):
        self.update(
            project_id=project_id,
            graphic_id="tuya_ui_handlers_rps",
            name="Tuya: Методы UI - RPS",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.HttpGraphics.handler_rps, self.UI_HANDLERS))
        )
        self.update(
            project_id=project_id,
            graphic_id="tuya_ui_handlers_errors_rps",
            name="Tuya: Методы UI - RPS HTTP ошибок",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.HttpGraphics.handler_errors_rps, self.UI_HANDLERS)),
            extra_data={
                "min": 0,
                "max": 110,
            }
        )
        self.update(
            project_id=project_id,
            graphic_id="tuya_ui_handlers_errors_percent",
            name="Tuya: Методы UI - % HTTP ошибок",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.HttpGraphics.handler_errors_percent, self.UI_HANDLERS)),
            extra_data={
                "min": 0,
                "max": 110,
            }
        )
        self.update(
            project_id=project_id,
            graphic_id="tuya_ui_handlers_percentiles",
            name="Tuya: Методы UI - время, с (квантиль 99.9%)",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.HttpGraphics.handler_percentile, self.UI_HANDLERS)),
            extra_data={
                "min": 0,
                "max": 5,
            }
        )

    def update_ydb_graphics(self, project_id):
        self.update(
            project_id=project_id,
            graphic_id="tuya_ydb_methods_rps",
            name="Tuya: Методы YDB - RPS",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.DBGraphics.db_method_rps, self.YDB_METHODS))
        )
        self.update(
            project_id=project_id,
            graphic_id="tuya_ydb_methods_errors_rps",
            name="Tuya: Методы YDB - RPS Ошибок",
            parameters=self.PARAMETERS,
            expressions=list(
                map(
                    lambda method: ServiceGraphics.DBGraphics.db_method_errors_rps("ydb", method),
                    self.YDB_METHODS
                )
            ),
            extra_data={
                "min": 0,
                "max": 1,
            }
        )
        self.update(
            project_id=project_id,
            graphic_id="tuya_ydb_methods_errors_percent",
            name="Tuya: Методы YDB - RPS Ошибок",
            parameters=self.PARAMETERS,
            expressions=list(
                map(
                    lambda method: ServiceGraphics.DBGraphics.db_method_errors_percent("ydb", method),
                    self.YDB_METHODS
                )
            ),
            extra_data={
                "min": 0,
                "max": 1,
            }
        )
        self.update(
            project_id=project_id,
            graphic_id="tuya_ydb_methods_percentile",
            name="Tuya: Методы YDB - время, с (квантиль 99.9%)",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.DBGraphics.db_method_percentile, self.YDB_METHODS)),
            extra_data={
                "min": 0,
                "max": 5,
            }
        )

    def update_api_graphics(self, project_id):
        self.update(
            project_id=project_id,
            graphic_id="tuya_cloud_api_calls_rps",
            name="Tuya: Методы API - RPS",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.ApiCallGraphics.api_call_rps, self.TUYA_API_CALLS))
        )
        self.update(
            project_id=project_id,
            graphic_id="tuya_cloud_api_calls_errors",
            name="Tuya: Методы API - Ошибки, %",
            parameters=self.PARAMETERS,
            expressions=[
                ServiceGraphics.ApiCallGraphics.api_call_total_http_code_percent(self.CODES["4xx"], "tuya"),
                ServiceGraphics.ApiCallGraphics.api_call_total_http_code_percent(self.CODES["5xx"], "tuya"),
                ServiceGraphics.ApiCallGraphics.api_call_total_unavailable_percent(self.CODES["fails"], "tuya"),
            ],
            extra_data={
                "min": 0,
                "max": 1,
            }
        )
        self.update(
            project_id=project_id,
            graphic_id="tuya_cloud_api_calls_errors_rps",
            name="Tuya: Методы API - Ошибки, RPS",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.ApiCallGraphics.api_call_errors_rps, self.TUYA_API_CALLS)),
            extra_data={
                "min": 0,
                "max": 1,
            }
        )
        self.update(
            project_id=project_id,
            graphic_id="tuya_cloud_api_calls_errors_percent",
            name="Tuya: Методы API - Ошибки, %",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.ApiCallGraphics.api_call_errors_percent, self.TUYA_API_CALLS)),
            extra_data={
                "min": 0,
                "max": 110,
            }
        )
        self.update(
            project_id=project_id,
            graphic_id="tuya_cloud_api_calls_percentiles",
            name="Tuya: Методы API - время, с (квантиль 99.9%)",
            parameters=self.PARAMETERS,
            expressions=list(map(ServiceGraphics.ApiCallGraphics.api_call_percentile, self.TUYA_API_CALLS)),
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
            graphic_id="tuya_perf_goroutines_count",
            name="Tuya: Горутины - Кол-во",
            parameters=self.PERF_PARAMETERS,
            expressions=list(map(ServiceGraphics.PerfGraphics.goroutines_count, self.DC))
        )
        self.update(
            project_id=project_id,
            graphic_id="tuya_perf_total_sys_alloc",
            name="Tuya: Total sys - alloc, Mb",
            parameters=self.PERF_PARAMETERS,
            expressions=list(map(lambda dc: ServiceGraphics.PerfGraphics.total_sys_alloc(dc, 1024 * 1024), self.DC))
        )

    def update_perf_gc_graphics(self, project_id):
        self.update(
            project_id=project_id,
            graphic_id="tuya_perf_gc_count",
            name="Tuya: Циклы GC - Кол-во",
            parameters=self.PERF_PARAMETERS,
            expressions=list(map(ServiceGraphics.PerfGraphics.gc_count, self.DC)),
            extra_data={
                "min": "0",
            }
        )
        self.update(
            project_id=project_id,
            graphic_id="tuya_perf_gc_pause",
            name="Tuya: Циклы GC - Время, мс",
            parameters=self.PERF_PARAMETERS,
            expressions=list(map(ServiceGraphics.PerfGraphics.gc_pauses, self.DC)),
            extra_data={
                "min": "0",
            }
        )

    def update_perf_memory_graphics(self, project_id):
        self.update(
            project_id=project_id,
            graphic_id="tuya_perf_memory_heap_alloc",
            name="Tuya: Heap - Alloc, Mb",
            parameters=self.PERF_PARAMETERS,
            expressions=list(map(lambda dc: ServiceGraphics.PerfGraphics.heap_alloc(dc, 1024 * 1024), self.DC))
        )
        self.update(
            project_id=project_id,
            graphic_id="tuya_perf_memory_heap_objects",
            name="Tuya: Heap objects - Кол-во",
            parameters=self.PERF_PARAMETERS,
            expressions=list(map(ServiceGraphics.PerfGraphics.heap_objects, self.DC))
        )
        self.update(
            project_id=project_id,
            graphic_id="tuya_perf_memory_heap_released",
            name="Tuya: Heap - Released, Mb",
            parameters=self.PERF_PARAMETERS,
            expressions=list(map(lambda dc: ServiceGraphics.PerfGraphics.heap_released(dc, 1024 * 1024), self.DC))
        )
        self.update(
            project_id=project_id,
            graphic_id="tuya_perf_memory_heap_in_use",
            name="Tuya: Heap - In use, Mb",
            parameters=self.PERF_PARAMETERS,
            expressions=list(
                map(lambda dc: ServiceGraphics.PerfGraphics.memtype_in_use(dc, "heap", 1024 * 1024), self.DC))
        )
        self.update(
            project_id=project_id,
            graphic_id="tuya_perf_memory_stack_in_use",
            name="Tuya: Stack - In use, Mb",
            parameters=self.PERF_PARAMETERS,
            expressions=list(
                map(lambda dc: ServiceGraphics.PerfGraphics.memtype_in_use(dc, "stack", 1024 * 1024), self.DC))
        )

    def update_neighbour_graphics(self, project_id):
        def get_calls(neighbour):
            if neighbour == "blackbox":
                return self.BLACKBOX_API_CALLS
            elif neighbour == "tvm":
                return self.TVM_API_CALLS
            else:
                return {}

        for neighbour in ["blackbox", "tvm"]:
            self.update(
                project_id=project_id,
                graphic_id="tuya_{neighbour}_api_calls_rps".format(neighbour=neighbour.lower()),
                name="{neighbour}: Методы API - RPS".format(neighbour=neighbour.capitalize()),
                parameters=self.PARAMETERS,
                expressions=list(map(
                    lambda c: ServiceGraphics.NeighbourGraphics.call_rps(neighbour, c), get_calls(neighbour)
                )),
            )
            self.update(
                project_id=project_id,
                graphic_id="tuya_{neighbour}_api_errors".format(neighbour=neighbour.lower()),
                name="{neighbour}: Методы API - Ошибки, %".format(neighbour=neighbour.capitalize()),
                parameters=self.PARAMETERS,
                expressions=[
                    ServiceGraphics.NeighbourGraphics.call_total_http_code_percent(neighbour, self.CODES["4xx"]),
                    ServiceGraphics.NeighbourGraphics.call_total_http_code_percent(neighbour, self.CODES["5xx"]),
                    ServiceGraphics.NeighbourGraphics.call_total_unavailable_percent(neighbour, self.CODES["fails"]),
                ],
                extra_data={
                    "min": 0,
                    "max": 110,
                }
            )
            for call in get_calls(neighbour):
                self.update(
                    project_id=project_id,
                    graphic_id="tuya_{neighbour}_api_call_{call}_rps".format(
                        neighbour=neighbour.lower(), call=call.name
                    ),
                    name="{neighbour}: Методы API - {call}, RPS".format(neighbour=neighbour.capitalize(),
                                                                        call=call.name),
                    parameters=self.PARAMETERS,
                    expressions=[
                        ServiceGraphics.NeighbourGraphics.call_unavailable_rps(neighbour, call, self.CODES["fails"]),
                        ServiceGraphics.NeighbourGraphics.call_http_code_rps(neighbour, call, self.CODES["4xx"]),
                        ServiceGraphics.NeighbourGraphics.call_http_code_rps(neighbour, call, self.CODES["5xx"]),
                        ServiceGraphics.NeighbourGraphics.call_http_code_rps(neighbour, call, self.CODES["2xx"]),
                    ]
                )
                self.update(
                    project_id=project_id,
                    graphic_id="tuya_{neighbour}_api_call_{call}_percent".format(
                        neighbour=neighbour.lower(), call=call.name
                    ),
                    name="{neighbour}: Методы API - {call}, %".format(neighbour=neighbour.capitalize(), call=call.name),
                    parameters=self.PARAMETERS,
                    expressions=[
                        ServiceGraphics.NeighbourGraphics.call_unavailable_rps(neighbour, call, self.CODES["fails"]),
                        ServiceGraphics.NeighbourGraphics.call_http_code_rps(neighbour, call, self.CODES["4xx"]),
                        ServiceGraphics.NeighbourGraphics.call_http_code_rps(neighbour, call, self.CODES["5xx"]),
                        ServiceGraphics.NeighbourGraphics.call_http_code_rps(neighbour, call, self.CODES["2xx"]),
                    ],
                    extra_data={
                        "normalize": True,
                        "min": 0,
                        "max": 110,
                    }
                )
            self.update(
                project_id=project_id,
                graphic_id="tuya_{neighbour}_api_call_percentiles".format(neighbour=neighbour.lower()),
                name="{neighbour}: Методы API - Тайминги, с (квантиль 99.9%)".format(neighbour=neighbour.capitalize()),
                parameters=self.PARAMETERS,
                expressions=list(map(
                    lambda c: ServiceGraphics.NeighbourGraphics.call_percentile(neighbour, c), get_calls(neighbour)
                )),
                extra_data={
                    "interpolate": "RIGHT",
                }
            )

    def update_by_dc_graphics(self, project_id):
        self.update(
            project_id=project_id,
            graphic_id="tuya_ui_rps_stacked_dc",
            name="Tuya: Mobile RPS stacked by DC",
            parameters=self.BY_DC_PARAMETERS,
            expressions=list(
                map(
                    lambda dc: ServiceGraphics.ByDCGraphics.total_path_rps_in_dc(dc.with_area(), "/m/*"), self.DC
                )
            )
        )
        for code in ["4xx", "5xx"]:
            self.update(
                project_id=project_id,
                graphic_id="tuya_ui_{code}_rps_stacked_dc".format(code=code),
                name="Tuya: Mobile {code} stacked by DC, RPS".format(code=code),
                parameters=self.BY_DC_PARAMETERS,
                expressions=list(
                    map(
                        lambda dc: ServiceGraphics.ByDCGraphics.error_code_rps_in_dc(dc, code, "/m/*"), self.DC
                    )
                ),
                extra_data={
                    "min": 0,
                    "max": 2,
                }
            )
            self.update(
                project_id=project_id,
                graphic_id="tuya_ui_{code}_percent_stacked_dc".format(code=code),
                name="Tuya: Mobile {code} stacked by DC, %".format(code=code),
                parameters=self.BY_DC_PARAMETERS,
                expressions=list(
                    map(
                        lambda dc: ServiceGraphics.ByDCGraphics.error_code_percent_in_dc(dc, code, "/m/*"), self.DC
                    )
                ),
                extra_data={
                    "min": 0,
                    "max": 110,
                }
            )
        for dc in self.DC:
            self.update(
                project_id=project_id,
                graphic_id="tuya_ui_handlers_percentiles_dc_{host}".format(host=dc.name.lower()),
                name="Tuya: Методы UI - время {host}, с (квантиль 99.9%)".format(host=dc.name.upper()),
                parameters=self.BY_DC_PARAMETERS,
                expressions=list(
                    map(lambda handler: ServiceGraphics.ByDCGraphics.handler_percentile_in_dc(handler, dc.name),
                        self.UI_HANDLERS)),
                extra_data={
                    "min": 0,
                    "max": 35,
                }
            )


class TuyaDashboards(DashboardBuilder):
    PROJECT = "alice-iot"
    CLUSTER = "tuya_production|tuya_beta"
    SERVICE = "tuya"

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
    BY_DC_PARAMETERS = [
        {
            "name": "project",
            "value": PROJECT,
        },
        {
            "name": "cluster",
            "value": "tuya_production",
        },
        {
            "name": "service",
            "value": SERVICE,
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

    def __init__(self, oauth_token):
        DashboardBuilder.__init__(self, oauth_token)

    def update_dashboards(self, project_id):
        # self.update_service_dashboard(project_id)
        # self.update_service_by_dc_dashboard(project_id)
        self.update_api_dashboard(project_id)
        # self.update_perf_dashboard(project_id)
        # self.update_neighbours_dashboard(project_id)

    def update_service_dashboard(self, project_id):
        self.update(
            project_id=project_id,
            dashboard_id="tuya_service_dashboard",
            name="Tuya: Service stats",
            parameters=self.PARAMETERS,
            rows=[
                [
                    Panel(
                        title="Методы UI - RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "tuya_ui_handlers_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы UI - RPS HTTP Ошибок",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "tuya_ui_handlers_errors_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы UI - % HTTP Ошибок",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "tuya_ui_handlers_errors_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы UI - время, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "tuya_ui_handlers_percentiles",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
                [
                    Panel(
                        title="Методы YDB - RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "tuya_ydb_methods_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=4,
                    ),
                    Panel(
                        title="Методы YDB - RPS Ошибок",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "tuya_ydb_methods_errors_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=4,
                    ),
                    Panel(
                        title="Методы YDB - время, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "tuya_ydb_methods_percentile",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=4,
                    ),
                ],
            ]
        )

    def update_service_by_dc_dashboard(self, project_id):
        self.update(
            project_id=project_id,
            dashboard_id="tuya_service_by_dc_dashboard",
            name="Tuya: Service stats by DC",
            parameters=self.BY_DC_PARAMETERS,
            rows=[
                [
                    Panel(
                        title="Методы UI - RPS stacked",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "graph", "tuya_ui_rps_stacked_dc",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=6,
                    ),
                ],
                [
                    Panel(
                        title="Методы UI - 4xx RPS stacked",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "graph", "tuya_ui_4xx_rps_stacked_dc",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы UI - 4xx % stacked",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "graph", "tuya_ui_4xx_percent_stacked_dc",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
                [
                    Panel(
                        title="Методы UI - 5xx RPS stacked",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "graph", "tuya_ui_5xx_rps_stacked_dc",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы UI - 5xx % stacked",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "graph", "tuya_ui_5xx_percent_stacked_dc",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
                [
                    Panel(
                        title="Методы UI - время MAN, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "graph", "tuya_ui_handlers_percentiles_dc_man",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы UI - время SAS, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "graph", "tuya_ui_handlers_percentiles_dc_sas",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы UI - время VLA, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "graph", "tuya_ui_handlers_percentiles_dc_vla",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ],
            ]
        )

    def update_api_dashboard(self, project_id):
        self.update(
            project_id=project_id,
            dashboard_id="tuya_api_dashboard",
            name="Tuya: API stats",
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
                            "graph", "tuya_cloud_api_calls_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы API - Всего ошибок, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "tuya_cloud_api_calls_errors",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы API - Тайминги, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "tuya_cloud_api_calls_percentiles",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ],
                [
                    Panel(
                        title="Методы API - Ошибки, RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "tuya_cloud_api_calls_errors_rps",
                            "scale", "log",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы API - Ошибки, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "tuya_cloud_api_calls_errors_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
            ]
        )

    def update_perf_dashboard(self, project_id):
        self.update(
            project_id=project_id,
            dashboard_id="tuya_perf_dashboard",
            name="Tuya: Perf stats",
            parameters=self.PERF_PARAMETERS,
            rows=[
                [
                    Panel(
                        title="Total sys - Alloc, mb",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "graph", "tuya_perf_total_sys_alloc",
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
                            "graph", "tuya_perf_gc_count",
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
                            "graph", "tuya_perf_gc_pause",
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
                            "graph", "tuya_perf_goroutines_count",
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
                            "graph", "tuya_perf_memory_heap_in_use",
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
                            "graph", "tuya_perf_memory_heap_alloc",
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
                            "graph", "tuya_perf_memory_heap_released",
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
                            "graph", "tuya_perf_memory_heap_objects",
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
                            "graph", "tuya_perf_memory_stack_in_use",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                ],
            ]
        )

    def update_neighbours_dashboard(self, project_id):
        self.update_blackbox_dashboards(project_id)
        self.update_tvm_dashboards(project_id)

    def update_blackbox_dashboards(self, project_id):
        self.update(
            project_id=project_id,
            dashboard_id="tuya_blackbox_dashboard",
            name="Tuya: Blackbox stats",
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
                            "graph", "tuya_blackbox_api_calls_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы API - Ошибки, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "tuya_blackbox_api_errors",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы API - Тайминги, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "tuya_blackbox_api_call_percentiles",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ],
                [
                    Panel(
                        title="Методы API - sessionID, RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "tuya_blackbox_api_call_sessionID_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы API - sessionID, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "tuya_blackbox_api_call_sessionID_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
                [
                    Panel(
                        title="Методы API - oauth, RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "tuya_blackbox_api_call_oauth_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы API - oauth, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "tuya_blackbox_api_call_oauth_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
            ]
        )

    def update_tvm_dashboards(self, project_id):
        self.update(
            project_id=project_id,
            dashboard_id="tuya_tvm_dashboard",
            name="Tuya: Tvm stats",
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
                            "skill_id", "{{skill_id}}",
                            "graph", "tuya_tvm_api_calls_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы API - Ошибки, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "graph", "tuya_tvm_api_errors",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                    Panel(
                        title="Методы API - Тайминги, с (квантиль 99.9%)",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "graph", "tuya_tvm_api_call_percentiles",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=2,
                    ),
                ],
                [
                    Panel(
                        title="Методы API - getServiceTicket, RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "graph", "tuya_tvm_api_call_getServiceTicket_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы API - getServiceTicket, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "graph", "tuya_tvm_api_call_getServiceTicket_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
                [
                    Panel(
                        title="Методы API - checkServiceTicket, RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "graph", "tuya_tvm_api_call_checkServiceTicket_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы API - checkServiceTicket, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "skill_id", "{{skill_id}}",
                            "graph", "tuya_tvm_api_call_checkServiceTicket_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
                [
                    Panel(
                        title="Методы API - checkUserTicket, RPS",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "tuya_tvm_api_call_checkUserTicket_rps",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                    Panel(
                        title="Методы API - checkUserTicket, %",
                        url=DashboardBuilder.url_query(
                            "project", project_id,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "graph", "tuya_tvm_api_call_checkUserTicket_percent",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=3,
                    ),
                ],
            ]
        )
