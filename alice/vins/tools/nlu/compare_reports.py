# coding: utf-8
from __future__ import unicode_literals

import logging
import click


from vins_core.logger import set_default_logging

from vins_tools.nlu.inspection.results_comparison import (
    compare_reports, report_and_exit_error_stats, do_compare_p_values
)

click.disable_unicode_literals_warning = True

logger = logging.getLogger(__name__)


@click.group()
@click.option('--log-level', type=click.Choice(('DEBUG', 'INFO', 'WARNING', 'ERROR')), default='INFO')
@click.pass_context
def main(ctx, log_level):
    set_default_logging(log_level)


@main.command('compare')
@click.argument('report_reference', type=click.Path(exists=True))
@click.argument('report_new', type=click.Path(exists=True))
@click.option('--metric', '-m', help='additional metrics to check', multiple=True)
@click.option('--delta', '-d', help='error level for delta for metrics decreasing', default=0.01, type=click.FLOAT)
@click.option('--error-stats', help='If specified, error stats dumped to this file without failing', default=None)
@click.pass_context
def compare(ctx, report_reference, report_new, metric, delta, error_stats):
    compare_reports(
        report_reference=report_reference,
        report_new=report_new,
        metrics=metric,
        delta=delta,
        error_stats=error_stats
    )


@main.command('report_and_exit')
@click.argument('errors_stats', type=click.Path(exists=False))
@click.pass_context
def report_and_exit(ctx, errors_stats):
    report_and_exit_error_stats(errors_stats)


@main.command('compare_p_values')
@click.argument('report', type=click.Path(exists=True))
@click.option('--metrics', '-m', help='Additional metrics to check', multiple=True,
              default=['recall_pv', 'f1_measure_pv'])
@click.option('--min-support', '-s', help='Minimal support for an intent to be counted', default=30, type=click.INT)
@click.option('--critical-value', '-p', help='Critical value for failing the task', default=0.05, type=click.FLOAT)
@click.option('--error-stats', help='If specified, error stats dumped to this file without failing', default=None)
@click.pass_context
def compare_p_values(ctx, report, metrics, min_support, critical_value, error_stats):
    do_compare_p_values(report, metrics, min_support, critical_value, error_stats)


def do_main():
    main(obj={})


if __name__ == '__main__':
    do_main()
