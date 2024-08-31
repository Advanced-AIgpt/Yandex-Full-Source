# -*- coding: utf8 -*-
# flake8: noqa
"""
IOT Service graphics
"""
import re
from collections import namedtuple

import attr

from alice.iot.dashboards.lib.solomon.aql import AQL
from alice.iot.dashboards.lib.solomon.graphic_builder import GraphicBuilder, Expression

HttpHandler = namedtuple('HttpHandler', ['path', 'method'])


@attr.s
class NamedProcessor(object):
    code_name = attr.ib()
    view_name = attr.ib()


def path_to_title(path):
    title = re.sub(r"{.*?}", "@", path)
    return title


class ServiceGraphics(GraphicBuilder):
    class HttpGraphics(GraphicBuilder):
        @staticmethod
        def handler_rps(handler):
            expression = '''
            {{
                sensor="handlers.calls",
                path="{path}",
                http_method="{http_method}"
            }}
            '''.format(path=handler.path, http_method=handler.method)
            return Expression(
                expression=AQL.aggregate_expression("sum", expression, "path"),
                title="{path} - {method}".format(path=path_to_title(handler.path), method=handler.method)
            )

        @staticmethod
        def handler_errors_rps(handler):
            errors = '''
            {{
                sensor="handlers.calls",
                path="{path}",
                http_method="{http_method}",
                http_code!="2xx"
            }}
            '''.format(path=handler.path, http_method=handler.method)
            errors_aggregate = AQL.group_lines("sum", errors)
            return Expression(
                expression=errors_aggregate,
                title="{path} - {method}".format(path=path_to_title(handler.path), method=handler.method),
                area=False,
            )

        @staticmethod
        def handler_errors_in_period(handler, period):
            errors = '''
            {{
                sensor="handlers.calls",
                path="{path}",
                http_method="{http_method}",
                http_code!="2xx"
            }}
            '''.format(path=handler.path, http_method=handler.method)
            errors_aggregate_rps = AQL.group_lines("sum", errors)
            errors_aggregate = AQL.diff_expression(AQL.integrate_expression(errors_aggregate_rps))
            errors_aggregate_in_period = AQL.aggregate_expression("sum", errors_aggregate, period)
            return Expression(
                expression=errors_aggregate_in_period,
                title="{path} - {method}".format(path=path_to_title(handler.path), method=handler.method),
                area=True,
                stack=True,
            )

        @staticmethod
        def handler_errors_percent(handler):
            errors = '''
            {{
                sensor="handlers.calls",
                path="{path}",
                http_method="{http_method}",
                http_code!="2xx"
            }}
            '''.format(path=handler.path, http_method=handler.method)
            errors_aggregate = AQL.aggregate_expression("sum", errors, "path")

            total = '''
            {{
                sensor="handlers.calls",
                path="{path}",
                http_method="{http_method}"
            }}
            '''.format(path=handler.path, http_method=handler.method)
            total_aggregate = AQL.aggregate_expression("sum", total, "path")
            return Expression(
                expression=AQL.percent_expression(errors_aggregate, total_aggregate),
                title="{path} - {method}".format(path=path_to_title(handler.path), method=handler.method),
                area=False,
            )

        @staticmethod
        def handler_percentile(handler):
            expression = '''
            {{
                sensor="handlers.duration_buckets",
                bin!="inf",
                path="{path}",
                http_method="{http_method}"
            }}
            '''.format(path=handler.path, http_method=handler.method)
            labels = ["path"]
            return Expression(
                expression=AQL.group_by_labels(expression, AQL.as_vector(labels), AQL.histogram_percentile_lambda()),
                title="{path} - {method}".format(path=path_to_title(handler.path), method=handler.method),
                area=False,
            )

    class ByDCGraphics(GraphicBuilder):
        @staticmethod
        def total_path_rps_in_dc(dc, path):
            expression = '''
            {{
                sensor="handlers.calls",
                path="{path}",
                host="{dc}"
            }}
            '''.format(dc=dc.name, path=path)
            return Expression(
                expression=AQL.aggregate_expression("sum", expression, "host"),
                title=dc.name,
                color=dc.color,
                area=dc.area,
            )

        @staticmethod
        def error_code_rps_in_dc(dc, http_code, path):
            errors = '''
            {{
                sensor="handlers.calls",
                host="{dc}",
                path="{path}",
                http_code="{http_code}"
            }}
            '''.format(dc=dc.name, http_code=http_code, path=path)
            errors_aggregate = AQL.group_lines("sum", errors)
            return Expression(
                expression=errors_aggregate,
                title=dc.name,
                color=dc.color,
                area=dc.area,
            )

        @staticmethod
        def error_code_percent_in_dc(dc, http_code, path):
            errors = '''
            {{
                sensor="handlers.calls",
                host="{dc}",
                path="{path}",
                http_code="{http_code}"
            }}
            '''.format(dc=dc.name, http_code=http_code, path=path)
            errors_aggregate = AQL.aggregate_expression("sum", errors, "host")

            total = '''
            {{
                sensor="handlers.calls",
                host="{dc}",
                path="{path}"
            }}
            '''.format(dc=dc.name, path=path)
            total_aggregate = AQL.aggregate_expression("sum", total, "host")
            return Expression(
                expression=AQL.percent_expression(errors_aggregate, total_aggregate),
                title=dc.name,
                color=dc.color,
                area=dc.area,
            )

        @staticmethod
        def handler_percentile_in_dc(handler, dc):
            expression = '''
            {{
                sensor="handlers.duration_buckets",
                bin!="inf",
                path="{path}",
                host="{dc}",
                http_method="{http_method}"
            }}
            '''.format(path=handler.path, http_method=handler.method, dc=dc)
            labels = ["path"]
            return Expression(
                expression=AQL.group_by_labels(expression, AQL.as_vector(labels), AQL.histogram_percentile_lambda()),
                title="{path} - {method}".format(path=path_to_title(handler.path), method=handler.method),
                # color=handler.color,
                area=False,
            )

    class ProcessorGraphics(GraphicBuilder):
        @staticmethod
        def errors_rps(named_processor):
            errors_rps = '''
                {{
                    sensor="processor.requests",
                    processor="{processor}",
                    command_status!="ok|total"
                }}
                '''.format(processor=named_processor.code_name)
            request = AQL.aggregate_expression("sum", errors_rps, "command_status")
            return Expression(
                expression=request,
                title="{processor}".format(processor=named_processor.view_name),
                area=True,
            )

        @staticmethod
        def processor_rps(named_processor):
            request_rps = '''
                {{
                    sensor="processor.requests",
                    processor="{processor}",
                    command_status="total"
                }}
                '''.format(processor=named_processor.code_name)
            return Expression(
                expression=request_rps,
                title="{processor}".format(processor=named_processor.view_name),
                area=True,
            )

        @staticmethod
        def processor_percentile(named_processor):
            bins = '''
            {{
                sensor="processor.duration_buckets",
                bin!="inf",
                processor="{processor}"
            }}
            '''.format(processor=named_processor.code_name)
            return Expression(
                expression=AQL.histogram_percentile(99.9, bins),
                title="{processor}".format(processor=named_processor.view_name),
                area=False,
            )

    class ProviderGraphics(GraphicBuilder):
        @staticmethod
        def request(command_status):
            request_rps = '''
                request{{
                    command_status={status}
                 }}
             '''.format(status=command_status.name)
            request = AQL.diff_expression(AQL.integrate_expression(request_rps))
            return Expression(
                expression=request,
                title=command_status.name,
                color=command_status.color,
                area=command_status.area,
            )

        @staticmethod
        def request_in_period(command_status, period):
            request_rps = '''
                request{{
                    command_status={status}
                 }}
             '''.format(status=command_status.name)
            request = AQL.diff_expression(AQL.integrate_expression(request_rps))
            request_in_period = AQL.aggregate_expression("sum", request, period)
            return Expression(
                expression=request_in_period,
                title=command_status.name,
                color=command_status.color,
                area=command_status.area,
            )

        @staticmethod
        def request_total(command):
            request_rps = '''
                request{{
                    command={command},
                    command_status=total
                 }}
             '''.format(command=command.name)
            request = AQL.diff_expression(AQL.integrate_expression(request_rps))
            return Expression(
                expression=request,
                title=command.name,
                color=command.color,
                area=command.area,
            )

        @staticmethod
        def request_total_http_percent(command):
            request_http_errors_lines = '''
                request{{
                    command="{command}",
                    command_status="error_http_*"
                 }}
             '''.format(command=command.name)
            request_http_errors_rps = AQL.group_lines("sum", request_http_errors_lines)
            total_rps = '''
                request{{
                    command={command},
                    command_status=total
                }}
            '''.format(command=command.name)
            percent = AQL.percent_expression(request_http_errors_rps, total_rps)
            return Expression(
                expression=percent,
                title=command.name,
                color=command.color,
                area=command.area,
            )

        @staticmethod
        def request_total_in_period(command, period):
            request_rps = '''
                request{{
                    command={command},
                    command_status=total
                 }}
             '''.format(command=command.name)
            request = AQL.diff_expression(AQL.integrate_expression(request_rps))
            request_in_period = AQL.aggregate_expression("sum", request, period)
            return Expression(
                expression=request_in_period,
                title=command.name,
                color=command.color,
                area=command.area,
            )

        @staticmethod
        def protocol_answers_model_manufacturer(command, command_status):
            protocol_answers_rps = '''
                per_model_request{{
                    command={command},
                    command_status={status}
                }}
            '''.format(command=command, status=command_status.name)
            protocol_answers = AQL.diff_expression(AQL.integrate_expression(protocol_answers_rps))
            return Expression(
                expression=protocol_answers,
                title=command_status.name,
                color=command_status.color,
                area=command_status.area,
            )

        @staticmethod
        def protocol_callback_answers(handler, command_status):
            protocol_answers_rps = '''
                callback_request{{
                    handler={handler},
                    command_status={status}
                }}
            '''.format(handler=handler, status=command_status.name)
            protocol_answers = AQL.diff_expression(AQL.integrate_expression(protocol_answers_rps))
            return Expression(
                expression=protocol_answers,
                title=command_status.name,
                color=command_status.color,
                area=command_status.area,
            )

        @staticmethod
        def protocol_callbacks_per_skill_id():
            tuya_callback_handlers_rps = '''
                callback_request{ command_status="total", skill_id="T" }
            '''
            quasar_callback_handlers_rps = '''
                callback_request{ command_status="total", skill_id="Q" }
            '''
            xiaomi_callback_handlers_rps = '''
                callback_request{ command_status="total", skill_id="ad26f8c2-fc31-4928-a653-d829fda7e6c2" }
            '''
            other_callback_handlers_rps = '''
                callback_request{ command_status="total", skill_id!="T|Q|ad26f8c2-fc31-4928-a653-d829fda7e6c2|all" }
            '''
            return [
                Expression(
                    expression=AQL.aggregate_expression("sum", tuya_callback_handlers_rps, "command_status"),
                    title="T",
                    area=True,
                ),
                Expression(
                    expression=AQL.aggregate_expression("sum", quasar_callback_handlers_rps, "command_status"),
                    title="Q",
                    area=True,
                ),
                Expression(
                    expression=AQL.aggregate_expression("sum", xiaomi_callback_handlers_rps, "command_status"),
                    title="Xiaomi",
                    area=True,
                ),
                Expression(
                    expression=AQL.aggregate_expression("sum", other_callback_handlers_rps, "command_status"),
                    title="Other",
                    area=True,
                )
            ]

        @staticmethod
        def protocol_answers(command, command_status):
            protocol_answers_rps = '''
                per_device_request{{
                    command={command},
                    command_status={status}
                }}
            '''.format(command=command, status=command_status.name)
            protocol_answers = AQL.diff_expression(AQL.integrate_expression(protocol_answers_rps))
            return Expression(
                expression=protocol_answers,
                title=command_status.name,
                color=command_status.color,
                area=command_status.area,
            )

        @staticmethod
        def protocol_answers_in_period(command, command_status, period):
            protocol_answers_rps = '''
                per_device_request{{
                    command={command},
                    command_status={status}
                }}
            '''.format(command=command, status=command_status.name)
            protocol_answers = AQL.diff_expression(AQL.integrate_expression(protocol_answers_rps))
            protocol_answers_in_period = AQL.aggregate_expression("sum", protocol_answers, period)
            return Expression(
                expression=protocol_answers_in_period,
                title=command_status.name,
                color=command_status.color,
                area=command_status.area,
            )

        @staticmethod
        def percentile(command):
            bins = '''
                request_duration{{
                    command={command},
                    bin!="inf"
                }}'''.format(command=command.name)
            percentile = AQL.histogram_percentile(99.9, bins)
            return Expression(
                expression=percentile,
                title=command.name,
                color=command.color,
                area=command.area,
            )

        @staticmethod
        def percentiles_command(percentile):
            bins = '''
                request_duration{
                    bin!="inf"
                }'''
            return Expression(
                expression=AQL.histogram_percentile(percentile.name, bins),
                title=percentile.name,
                color=percentile.color,
                area=False,
            )

    class DBGraphics(GraphicBuilder):
        @staticmethod
        def cache_total_rps():
            expression = '''
            {
                sensor="repository.cache.total"
            }
            '''
            return Expression(
                expression=expression,
                title="Total"
            )

        @staticmethod
        def cache_stat_rps(stat):
            cache_stat = '''
            {{
                sensor="repository.cache.{stat}"
            }}
            '''.format(stat=stat)
            return Expression(
                expression=cache_stat,
                title="{stat}".format(stat=stat.capitalize()),
                area=True,
            )

        @staticmethod
        def cache_stat_percent(stat):
            part = '''
            {{
                sensor="repository.cache.{stat}"
            }}
            '''.format(stat=stat)
            total = '''
            {
                sensor="repository.cache.total"
            }
            '''
            percent = AQL.percent_expression(part, total)
            return Expression(
                expression=percent,
                title="{stat}".format(stat=stat.capitalize()),
                area=False,
            )

        @staticmethod
        def db_method_rps(db_method):
            expression = '''
            {{
                sensor="db.total",
                db_method="{db_method}"
            }}
            '''.format(db_method=db_method)
            return Expression(
                expression=expression,
                title=path_to_title(db_method)
            )

        @staticmethod
        def db_method_errors_rps(db_type, db_method):
            errors = '''
            {{
                sensor="db.fails",
                error_type="{db_type}_total",
                db_method="{db_method}"
            }}
            '''.format(db_type=db_type, db_method=db_method)
            return Expression(
                expression=errors,
                title=path_to_title(db_method),
            )

        @staticmethod
        def db_method_errors_percent(db_type, db_method):
            errors = '''
            {{
                sensor="db.fails",
                error_type="{db_type}_total",
                db_method="{db_method}"
            }}
            '''.format(db_type=db_type, db_method=db_method)

            total = '''
            {{
                sensor="db.total",
                db_method="{db_method}"
            }}
            '''.format(db_method=db_method)
            return Expression(
                expression=AQL.percent_expression(errors, total),
                title=path_to_title(db_method),
                area=False,
            )

        @staticmethod
        def db_method_percentile(db_method):
            expression = '''
            {{
                sensor="db.duration_buckets",
                bin!="inf",
                db_method="{db_method}"
            }}
            '''.format(db_method=db_method)
            labels = ["path"]
            return Expression(
                expression=AQL.group_by_labels(expression, AQL.as_vector(labels), AQL.histogram_percentile_lambda()),
                title=path_to_title(db_method),
                area=False,

            )

    class CallbackGraphics(GraphicBuilder):
        @staticmethod
        def callback_method_rps(callback):
            expression = '''
            {{
                sensor="callback.total",
                callback="{callback}"
            }}
            '''.format(callback=callback)
            return Expression(
                expression=expression,
                title=path_to_title(callback)
            )

        @staticmethod
        def callback_method_errors_rps(callback):
            errors = '''
            {{
                sensor="callback.fails",
                error_type="callback_total",
                callback="{callback}"
            }}
            '''.format(callback=callback)
            return Expression(
                expression=errors,
                title=path_to_title(callback),
            )

        @staticmethod
        def callback_method_percentile(callback):
            expression = '''
            {{
                sensor="callback.duration_buckets",
                bin!="inf",
                callback="{callback}"
            }}
            '''.format(callback=callback)
            labels = ["path"]
            return Expression(
                expression=AQL.group_by_labels(expression, AQL.as_vector(labels), AQL.histogram_percentile_lambda()),
                title=path_to_title(callback),
                area=False,

            )

    class PerfGraphics(GraphicBuilder):
        @staticmethod
        def gc_count(cluster):
            gc_completed = '''
                perf.gc.cycles{{
                    host={cluster}
                }}
            '''.format(cluster=cluster.name)
            gc_completed_diff = AQL.diff_expression(gc_completed)
            return Expression(
                expression=gc_completed_diff,
                title=cluster.name,
                color=cluster.color,
                area=cluster.area,
            )

        @staticmethod
        def gc_pauses(cluster):
            gc_pauses = '''
                perf.gc.pause_total_ns{{
                    host={cluster}
                }} / 1000000
            '''.format(cluster=cluster.name)
            gc_pauses_diff = AQL.diff_expression(gc_pauses)
            return Expression(
                expression=gc_pauses_diff,
                title=cluster.name,
                color=cluster.color,
                area=cluster.area,
            )

        @staticmethod
        def goroutines_count(cluster):
            goroutines_count = '''
                perf.general.goroutines{{
                    host={cluster}
                }}
            '''.format(cluster=cluster.name)
            return Expression(
                expression=goroutines_count,
                title=cluster.name,
                color=cluster.color,
                area=cluster.area,
            )

        @staticmethod
        def total_sys_alloc(cluster, measure):
            total_sys_alloc = '''
                perf.general.total_sys_bytes{{
                    host={cluster}
                }} / {measure}
            '''.format(cluster=cluster.name, measure=measure)
            return Expression(
                expression=total_sys_alloc,
                title=cluster.name,
                color=cluster.color,
                area=cluster.area,
            )

        @staticmethod
        def memtype_in_use(cluster, memtype, measure):
            memtype_in_use = '''
                perf.memory.in_use_bytes{{
                    host={cluster},
                    memory={memtype}
                }} / {measure}
            '''.format(cluster=cluster.name, memtype=memtype, measure=measure)
            return Expression(
                expression=memtype_in_use,
                title=cluster.name,
                color=cluster.color,
                area=cluster.area,
            )

        @staticmethod
        def heap_alloc(cluster, measure):
            heap_alloc = '''
                perf.memory.alloc_bytes{{
                    host={cluster},
                    memory=heap
                }} / {measure}
            '''.format(cluster=cluster.name, measure=measure)
            return Expression(
                expression=heap_alloc,
                title=cluster.name,
                color=cluster.color,
                area=cluster.area,
            )

        @staticmethod
        def heap_objects(cluster):
            heap_objects = '''
                perf.memory.objects{{
                    host={cluster},
                    memory=heap
                }}
            '''.format(cluster=cluster.name)
            return Expression(
                expression=heap_objects,
                title=cluster.name,
                color=cluster.color,
                area=cluster.area,
            )

        @staticmethod
        def heap_released(cluster, measure):
            heap_released = '''
                perf.memory.released_bytes{{
                    host={cluster},
                    memory=heap
                }} / {measure}
            '''.format(cluster=cluster.name, measure=measure)
            return Expression(
                expression=heap_released,
                title=cluster.name,
                color=cluster.color,
                area=cluster.area,
            )

    class NeighbourGraphics(GraphicBuilder):
        @staticmethod
        def call_rps(neighbour, call):
            calls_rps = '''
                neighbours.total{{
                    neighbour={neighbour},
                    call={call}
                }}
            '''.format(neighbour=neighbour, call=call.name)
            return Expression(
                expression=calls_rps,
                title=call.name,
                color=call.color,
                area=call.area,
            )

        @staticmethod
        def call_unavailable_rps(neighbour, call, code):
            unavailable_rps = '''
                neighbours.fails{{
                    neighbour={neighbour},
                    call={call}
                }}
            '''.format(neighbour=neighbour, call=call.name)
            return Expression(
                expression=unavailable_rps,
                title=code.name,
                color=code.color,
            )

        @staticmethod
        def call_http_code_rps(neighbour, call, code):
            http_code_rps = '''
                neighbours.calls{{
                    neighbour="{neighbour}",
                    call="{call}",
                    http_code="{code}"
                }}
            '''.format(neighbour=neighbour, call=call.name, code=code.name)
            return Expression(
                expression=http_code_rps,
                title=code.name,
                color=code.color,
            )

        @staticmethod
        def call_total_unavailable_percent(neighbour, code):
            unavailable_rps_lines = '''
                neighbours.fails{{
                    neighbour={neighbour}
                }}
            '''.format(neighbour=neighbour)
            unavailable_rps = AQL.aggregate_expression("sum", unavailable_rps_lines, "neighbour")
            total_rps_lines = '''
                neighbours.total{{
                    neighbour={neighbour}
                }}
            '''.format(neighbour=neighbour)
            total_rps = AQL.aggregate_expression("sum", total_rps_lines, "neighbour")
            total_unavailable_percent = AQL.percent_expression(unavailable_rps, total_rps)
            return Expression(
                expression=total_unavailable_percent,
                title=code.name,
                color=code.color,
            )

        @staticmethod
        def call_total_http_code_percent(neighbour, code):
            unavailable_rps_lines = '''
                neighbours.calls{{
                    neighbour={neighbour},
                    http_code={code}
                }}
            '''.format(neighbour=neighbour, code=code.name)
            unavailable_rps = AQL.aggregate_expression("sum", unavailable_rps_lines, "neighbour")
            total_rps_lines = '''
                neighbours.total{{
                    neighbour={neighbour}
                }}
            '''.format(neighbour=neighbour)
            total_rps = AQL.aggregate_expression("sum", total_rps_lines, "neighbour")
            total_unavailable_percent = AQL.percent_expression(unavailable_rps, total_rps)
            return Expression(
                expression=total_unavailable_percent,
                title=code.name,
                color=code.color,
            )

        @staticmethod
        def call_percentile(neighbour, call):
            expression = '''
                neighbours.duration_buckets{{
                    neighbour="{neighbour}",
                    bin!="inf",
                    call="{call}"
                }}
            '''.format(neighbour=neighbour, call=call.name)
            labels = ["call"]
            return Expression(
                expression=AQL.group_by_labels(expression, AQL.as_vector(labels), AQL.histogram_percentile_lambda()),
                title=call.name,
                area=False,
            )

    class ApiCallGraphics(GraphicBuilder):
        @staticmethod
        def api_call_rps(call):
            expression = '''
                {{
                    sensor="api.total",
                    call="{call}"
                }}
            '''.format(call=call.name)
            return Expression(
                expression=expression,
                title=call.name
            )

        @staticmethod
        def api_call_total_unavailable_percent(code, api):
            unavailable_rps_lines = '''
                {{
                    sensor="api.fails",
                    api="{api}"
                }}
            '''.format(api=api)
            unavailable_rps = AQL.group_lines("sum", unavailable_rps_lines)
            total_rps_lines = '''
                {{
                    sensor="api.total",
                    api="{api}"
                }}
            '''.format(api=api)
            total_rps = AQL.group_lines("sum", total_rps_lines)
            total_unavailable_percent = AQL.percent_expression(unavailable_rps, total_rps)
            return Expression(
                expression=total_unavailable_percent,
                title=code.name,
                color=code.color,
            )

        @staticmethod
        def api_call_total_http_code_percent(code, api):
            http_code_rps_lines = '''
                {{
                    sensor="api.calls",
                    api="{api}",
                    http_code={code}
                }}
            '''.format(code=code.name, api=api)
            http_code_rps = AQL.group_lines("sum", http_code_rps_lines)
            total_rps_lines = '''
                {{
                    sensor="api.total",
                    api="{api}"
                }}
            '''.format(api=api)
            total_rps = AQL.group_lines("sum", total_rps_lines)
            total_http_code_percent = AQL.percent_expression(http_code_rps, total_rps)
            return Expression(
                expression=total_http_code_percent,
                title=code.name,
                color=code.color,
            )

        @staticmethod
        def api_call_errors_rps(call):
            http_errors = '''
            {{
                sensor="api.calls",
                call="{call}",
                http_code!="2xx"
            }}
            '''.format(call=call.name)
            unavailable = '''
            {{
                sensor="api.fails",
                call="{call}"
            }}
            '''.format(call=call.name)
            errors = AQL.sum(AQL.group_lines("sum", http_errors), unavailable)
            return Expression(
                expression=errors,
                title=call.name,
                area=False,
            )

        @staticmethod
        def api_call_errors_percent(call):
            http_errors = '''
            {{
                sensor="api.calls",
                call="{call}",
                http_code!="2xx"
            }}
            '''.format(call=call.name)
            unavailable = '''
            {{
                sensor="api.fails",
                call="{call}"
            }}
            '''.format(call=call.name)
            errors = AQL.sum(AQL.group_lines("sum", http_errors), unavailable)

            total = '''
                {{
                    sensor="api.total",
                    call="{call}"
                }}
            '''.format(call=call.name)
            errors_percent = AQL.percent_expression(errors, total)
            return Expression(
                expression=errors_percent,
                title=call.name,
                area=False,
            )

        @staticmethod
        def api_call_percentile(call):
            expression = '''
                    {{
                        sensor="api.duration_buckets",
                        bin!="inf",
                        call="{call}"
                    }}
                    '''.format(call=call.name)
            return Expression(
                expression=AQL.histogram_percentile("99.9", expression),
                title=call.name,
                area=False,
            )

    class ApphostGraphics(GraphicBuilder):
        @staticmethod
        def handler_total(handler):
            expression = '''
                apphost.total{{
                    path="{handler}"
                }}
            '''.format(handler=handler.name)
            return Expression(
                expression=expression,
                title=handler.name,
                area=True,
            )

        @staticmethod
        def handler_fails(handler):
            fails = '''
                apphost.fails{{
                    path="{handler}"
                }}
            '''.format(handler=handler.name)
            return Expression(
                expression=fails,
                title=handler.name,
                area=False,
            )

        @staticmethod
        def handler_fails_percent(handler):
            fails = '''
                apphost.fails{{
                    path="{handler}"
                }}
            '''.format(handler=handler.name)
            total = '''
                apphost.total{{
                    path="{handler}"
                }}
            '''.format(handler=handler.name)
            fails_percent = AQL.percent_expression(fails, total)
            return Expression(
                expression=fails_percent,
                title=handler.name,
                area=False,
            )

        @staticmethod
        def handler_percentile(handler):
            expression = '''
                apphost.duration_buckets{{
                    bin!="inf",
                    path="{handler}"
                }}
            '''.format(handler=handler.name)
            labels = ["path"]
            return Expression(
                expression=AQL.group_by_labels(expression, AQL.as_vector(labels), AQL.histogram_percentile_lambda()),
                title=handler.name,
                area=False,
            )
