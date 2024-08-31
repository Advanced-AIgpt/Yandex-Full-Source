# -*- coding: utf8 -*-
# flake8: noqa
"""
IOT Queue graphics
"""

from alice.iot.dashboards.lib.solomon.aql import AQL
from alice.iot.dashboards.lib.solomon.graphic_builder import GraphicBuilder, Expression


class QueueGraphics(GraphicBuilder):
    @staticmethod
    def task_status(status_metric):
        expression = '''{{sensor="queue.{status}_tasks"}}'''.format(status=status_metric.name)
        return Expression(
            expression=expression,
            title="{status}".format(status=status_metric.name),
            color=status_metric.color,
            area=status_metric.area,
            stack=status_metric.stack,
        )

    @staticmethod
    def task_status_in_period(status_metric, period):
        task_status = '''{{sensor="queue.{status}_tasks"}}'''.format(status=status_metric.name)
        task_status_diff = AQL.diff_expression(task_status)
        expression = AQL.aggregate_expression("sum", task_status_diff, period)
        return Expression(
            expression=expression,
            title="{status}".format(status=status_metric.name),
            color=status_metric.color,
            area=status_metric.area,
            stack=status_metric.stack,
        )

    @staticmethod
    def average_overdue_time():
        overdue_diff = AQL.diff_expression('''{sensor="queue.overdue_time"}''')
        processed_tasks_diff = AQL.diff_expression('''{sensor="queue.processed_tasks"}''')
        expression = AQL.replace_nan(AQL.divide_expression(overdue_diff, processed_tasks_diff), 0)
        return Expression(
            expression=expression,
            title="Average overdue time",
            area=False,
        )

    @staticmethod
    def average_process_time():
        process_diff = AQL.diff_expression('''{sensor="queue.process_time"}''')
        processed_tasks_diff = AQL.diff_expression('''{sensor="queue.processed_tasks"}''')
        expression = AQL.replace_nan(AQL.divide_expression(process_diff, processed_tasks_diff), 0)
        return Expression(
            expression=expression,
            title="Average process time",
            area=False,
        )
