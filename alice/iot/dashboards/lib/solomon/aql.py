# -*- coding: utf8 -*-
# flake8: noqa
"""
Solomon analytics query language expressions
"""
import inspect


class AQL:
    current_value_variable_name = 'current_value'

    @staticmethod
    def alert_color_evaluation(is_red, is_yellow):
        program = '''
            let is_red = {is_red};
            let is_yellow = !is_red && {is_yellow};
            let trafficColor = is_red ? "red" : (is_yellow ? "yellow" : "green");

            alarm_if(is_red);
        '''.format(is_red=is_red, is_yellow=is_yellow)
        return inspect.cleandoc(program)

    @staticmethod
    def alert_if_gt(value, target):
        return f'let {AQL.current_value_variable_name} = {value};\nalarm_if({AQL.current_value_variable_name} >= {target});'

    @staticmethod
    def add_warn_if_gt(expression, target):
        return f'{expression}\nwarn_if({AQL.current_value_variable_name} >= {target});'

    @staticmethod
    def gt(expression, value):
        return '({expression} > {value})'.format(expression=expression, value=value)

    @staticmethod
    def logical_and(*args):
        return ' && '.join(args)

    @staticmethod
    def to_fixed(expression, n):
        return 'to_fixed({expression}, {n})'.format(expression=expression, n=n)

    @staticmethod
    def average(expression):
        return 'avg({expression})'.format(expression=expression)

    @staticmethod
    def max(expression):
        return 'max({expression})'.format(expression=expression)

    @staticmethod
    def split_timeseries(timeseries, n_head, n_tail):
        head = 'head({timeseries}, {nh})'.format(timeseries=timeseries, nh=n_head)
        mid = 'drop_head(drop_tail({timeseries}, {nt}), {nh})'.format(timeseries=timeseries, nh=n_head, nt=n_tail)
        tail = 'tail({timeseries}, {nt})'.format(timeseries=timeseries, nt=n_tail)
        return head, mid, tail

    @staticmethod
    def sum(left, right):
        return '({left} + {right})'.format(left=left, right=right)

    @staticmethod
    def sub(left, right):
        return '({left} - {right})'.format(left=left, right=right)

    @staticmethod
    def shift(expression, period):
        return 'shift({expression}, {period})'.format(expression=expression, period=period)

    @staticmethod
    def replace_nan(expression, value):
        return 'replace_nan({expression}, {value})'.format(expression=expression, value=value)

    @staticmethod
    def group_lines(aggr_fn, expression):
        return 'group_lines("{aggr_fn}", {expression})'.format(aggr_fn=aggr_fn, expression=expression)

    @staticmethod
    def aggregate_expression(aggr_fn, expression, tag):
        return '{aggr_fn}({expression}) by {tag}'.format(aggr_fn=aggr_fn, expression=expression, tag=tag)

    @staticmethod
    def combinate_expression(comb_fn, expression, tag=""):
        if tag:
            return f'{comb_fn}("{tag}", {expression})'

        return f'{comb_fn}({expression})'

    @staticmethod
    def divide_expression(numerator, denominator):
        return '{numerator} / {denominator}'.format(numerator=numerator, denominator=denominator)

    @staticmethod
    def multiply_expression(factor1, factor2):
        return '{factor1} * {factor2}'.format(factor1=factor1, factor2=factor2)

    @staticmethod
    def integrate_expression(expression):
        return 'integrate_fn({expression})'.format(expression=expression)

    @staticmethod
    def diff_expression(expression):
        return 'diff({expression})'.format(expression=expression)

    @staticmethod
    def percent_expression(part, total):
        return AQL.replace_nan(AQL.multiply_expression(AQL.divide_expression(part, total), 100), 0)

    @staticmethod
    def group_by_labels(expression, labels, fn):
        return 'group_by_labels({expression}, {labels}, {fn})'.format(expression=expression, labels=labels, fn=fn)

    @staticmethod
    def as_vector(labels):
        return ", ".join(map('"{}"'.format, labels))

    @staticmethod
    def histogram_percentile_lambda(percentile=99.9):
        return 'v -> histogram_percentile({percentile}, v)'.format(percentile=percentile)

    @staticmethod
    def histogram_percentile(percentile, bins):
        return 'histogram_percentile({percentile}, {bins})'.format(percentile=percentile, bins=bins)

    @staticmethod
    def arithmetic_average(*args):
        return '({expression}) / {n}'.format(expression=' + '.join(args), n=len(args))
