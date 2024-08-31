# -*- coding: utf-8 -*-
import os

from nile.api.v1 import (
    aggregators as na,
    datetime as nd,
    extractors as ne,
    cli,
    Record,
)

from qb2.api.v1 import (
    filters as qf,
    typing as qt,
)

from alice.analytics.operations.core_spu.utils import mapper_wrapper
from alice.analytics.operations.core_spu.activities import EXPBOXES_ACTIVITIES


CLI_OPTIONS = [
    cli.Option('days_recalc_interval', default='14'),  # should be greater than max(INTERVALS)
    cli.Option('job_mode', default='regular'),  # variants: 'regular', 'total_recalc'
    cli.Option('output_tag', default=''),
    cli.Option('recalc_period_start_date', default='2020-07-01'),  # only for total_recalc mode
    cli.Option('previous_table', default=None),  # only for regular mode
    cli.Option('corespu_path', default='//home/alice/cubes/corespu/'),  # path to main core_spu tables directory
]

PREPARED_LOGS_EXPBOXES_PATH = '//home/alice/dialog/prepared_logs_expboxes/{{{start_date}..{end_date}}}'

INTERVALS = (1, 3, 7)

ALL_STATIONS = frozenset(['quasar', 'small_smart_speakers'])
PARTNER_STATIONS = frozenset([
    'Dexp lightcomm', 'Elari elari_a98', 'Irbis linkplay_a98',
    'JBL jbl_link_music', 'JBL jbl_link_portable', 'LG wk7y',
    'Prestigio prestigio_smart_mate'
])


@mapper_wrapper
class CoreSPUReducer:
    output_schema = dict(
        device_id=qt.Optional[qt.String],
        device_type=qt.Optional[qt.String],
        activation_date=qt.Optional[qt.String],
        interval=qt.Optional[qt.UInt64],
        extended_intent=qt.Optional[qt.String],
        score=qt.Optional[qt.Float],
        score_raw=qt.Optional[qt.UInt64]
    )

    def __init__(self, job_date):
        self.job_date = job_date

    def __call__(self, groups):
        for key, records in groups:
            records = list(records)

            for interval in INTERVALS:
                # pre-filter records that match the interval/activation_date/fielddate combination
                if nd.next_day(key.activation_date, offset=(interval - 1), scale='daily') <= self.job_date:
                    records_for_interval = \
                        [r for r in records if
                         nd.distance_in_periods(key.activation_date, r.fielddate) < interval
                         ]
                    corespu_score = 0

                    for _activity in EXPBOXES_ACTIVITIES:
                        activity = _activity(device_id=key.device_id,
                                             device_type=key.device_type,
                                             app=key.app,
                                             activation_date=key.activation_date,
                                             interval=interval,
                                             job_date=self.job_date)

                        activity_score = activity.check_records_score(records_for_interval)
                        corespu_score += activity_score

                        yield Record(
                            device_id=key.device_id,
                            device_type=key.device_type,
                            activation_date=key.activation_date,
                            interval=interval,
                            extended_intent=activity.name,
                            score=activity_score,
                            score_raw=1 if activity_score > 0 else 0,  # unweighted raw score in 0..1 range
                        )
                    yield Record(
                        device_id=key.device_id,
                        device_type=key.device_type,
                        activation_date=key.activation_date,
                        interval=interval,
                        extended_intent='core_spu',
                        score=corespu_score,
                        score_raw=1,
                    )


@cli.statinfra_job(options=CLI_OPTIONS)
def make_job(job, options):
    job = job.env(auto_increase_memory_limit=True)

    job_date = options.dates[0]
    corespu_path = options.corespu_path
    output_tag = options.output_tag

    if options.job_mode == 'total_recalc':
        job_mode = 'total_recalc'
        recalc_period_start_date = options.recalc_period_start_date
        if output_tag == '':
            output_tag = '_recalc'
    else:
        job_mode = 'regular'
        days_recalc_interval = int(options.days_recalc_interval)
        recalc_period_start_date = nd.next_day(job_date, offset=-days_recalc_interval, scale='daily')
    previous_table = options.previous_table or nd.next_day(job_date, offset=-1, scale='daily')

    core_spu_features_per_device_id = job \
        .table(
            PREPARED_LOGS_EXPBOXES_PATH.format(start_date=recalc_period_start_date, end_date=job_date))\
        .filter(
            qf.defined('device_id', 'fielddate'),
            qf.compare('first_day', '>=', recalc_period_start_date),
            qf.custom(
                lambda first_day, fielddate:
                nd.distance_in_periods(first_day, fielddate) < max(INTERVALS),
                'first_day', 'fielddate'),
            qf.one_of('app', ALL_STATIONS)) \
        .project(
            'req_id',
            'device_id',
            'fielddate',
            'intent',
            'generic_scenario',
            'analytics_info',
            'music_answer_type',
            'app',
            'query',
            activation_date='first_day',
            device_type='device') \
        .groupby(
            'device_id',
            'device_type',
            'app',
            'activation_date') \
        .reduce(
            CoreSPUReducer(job_date)) \
        .unique('device_id', 'activation_date', 'device_type', 'interval', 'extended_intent', 'score_raw')

    # Результат 1: все фичи (включая нулевые) для каждого устройства в каждом интервале.
    # saving features and aggregate core_spu data; merging with old files

    if job_mode == 'total_recalc':
        core_spu_features_per_device_id.put(
            os.path.join(corespu_path, 'sources', job_date + output_tag),
            )
    else:
        old_core_spu_features = job \
            .table(os.path.join(corespu_path, 'sources', previous_table)) \
            .filter(qf.compare('activation_date', '<', recalc_period_start_date))

        core_spu_features_per_device_id = job \
            .concat(
                old_core_spu_features,
                core_spu_features_per_device_id) \
            .put(
                os.path.join(corespu_path, 'sources', job_date + output_tag),
                )

    # Результат 2: отдельные фичи, агрегированные по device_type и интервалу:

    core_spu_features_per_device_id \
        .project(ne.all(exclude=['device_type']),
                 device_type=ne.custom(
                     lambda device_type: 'partner_station' if device_type in PARTNER_STATIONS else device_type,
                     'device_type'
                     ).with_type(qt.Optional[qt.String]),
                 ) \
        .groupby('activation_date', 'device_type', 'interval', 'extended_intent') \
        .aggregate(
            feature_raw=na.mean('score_raw'),
            feature_score=na.mean('score'),
            ) \
        .put(os.path.join(corespu_path, 'core_spu_features_series' + output_tag))

    # Результат 3: только core_spu, агрегированное по device_type и интервалу:

    core_spu_features_per_device_id \
        .filter(qf.equals('extended_intent', 'core_spu')) \
        .project(ne.all(exclude=['extended_intent', 'score', 'device_type']),
                 core_spu='score',
                 device_type=ne.custom(
                     lambda device_type: 'partner_station' if device_type in PARTNER_STATIONS else device_type,
                     'device_type'
                     ).with_type(qt.Optional[qt.String])) \
        .groupby('activation_date', 'interval', 'device_type') \
        .aggregate(
            core_spu=na.mean('core_spu'),
            cohort_size=na.count()) \
        .put(os.path.join(corespu_path, 'core_spu_aggregate' + output_tag))
    return job


if __name__ == '__main__':
    cli.run()
