#!/usr/bin/env python
# encoding: utf-8

from utils.nirvana.op_caller import call_as_operation
from utils.yt.dep_manager import hahn_with_deps

from nile.api.v1 import (
    Record,
    aggregators as na,
    extractors as ne,
    with_hints,
)
from qb2.api.v1 import (
    typing as qt,
)
from datetime import timedelta, datetime


@with_hints(output_schema=dict(
    age=qt.Optional[qt.Float],
    name=qt.String,
))
def extract_user_information(rows):
    """Просто функция, преобразующая данные"""
    for row in rows:
        yield Record(
            age=row.age,
            name=row.name.decode('utf-8')
        )


def compute_metrics(start, end, report_config, prepared_root):
    """
    start, end - период, для которого нужно посчитать новые значения
    смотрит в логи [start - (30 + 6 + 28), end]
    """
    start_date = datetime.strptime(start, '%Y-%m-%d').date()
    # end_date = datetime.strptime(end, '%Y-%m-%d').date()
    log_start = (start_date - timedelta(30 + 6 + 28)).strftime("%Y-%m-%d")

    templates = dict(dates='{' + log_start + '..' + end + '}')

    cluster = hahn_with_deps(
        pool='voice',
        use_yql=True,
        templates=templates,
        # neighbours_for=__file__,
        # neighbour_names=None,
        # include_utils=True,
    )

    # для nile+python3 не собранного ya.make из аркадии, нужно тащить с собой porto-слои
    PYTHON3_LAYERS = [
        '//porto_layers/base/bionic/porto_layer_search_ubuntu_bionic_app_lastest.tar.gz',
        '//home/portolayer/bionic/python3/latest',
    ]
    job = cluster.job().env(
        yt_spec_defaults=dict(
            mapper=dict(layer_paths=PYTHON3_LAYERS),
            reducer=dict(layer_paths=PYTHON3_LAYERS),
        )
    )

    (job
        .table('//home/yql/tutorial/users')
        .map(extract_user_information)
        .groupby()
        .aggregate(
            age=na.mean('age'),
        )
        .project(
            ne.all(),
            some_const_field=ne.const('some_const_field_value').with_type(qt.String),
        )
        .put('//tmp/yql_tutorial_users_mean_age_2'))

    job.run()


def main(
    start,
    end,
    report_path='VoiceTech/Dialog/session_metrics/retention_by_scenario',
    prepared_root='//home/alice/dialog/prepared_logs_expboxes'
):
    # TODO: make_stat_client()
    report_config = None

    compute_metrics(start, end, report_config, prepared_root)


if __name__ == '__main__':
    call_as_operation(main)
