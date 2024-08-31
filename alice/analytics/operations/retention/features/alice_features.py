# -*- coding: utf-8 -*-

import os
from datetime import timedelta

from nile.api.v1 import (
    datetime as nd,
    with_hints,
    Record,
)

from qb2.api.v1 import (
    filters as qf,
    typing as qt,
)

from alice.analytics.operations.retention.constants import (
    ALICE_FEATURES_UPDATE_PERIOD,
    CHILD_CONFIDENCE_THRESHOLD,
    ALICE_FEATURES_PATH
)



@with_hints(output_schema=dict(
    user_id=qt.String,
    fielddate=qt.String,
    scenarios_features=qt.Yson,
    biometric_features=qt.Yson,
))
def alice_daily_features_reducer(groups):
    for key, records in groups:
        scenarios_features = set()
        biometric_features = set()

        for rec in records:
            scenarios_features.add(rec.generic_scenario)

            if rec.child_confidence is not None and rec.child_confidence > CHILD_CONFIDENCE_THRESHOLD:
                biometric_features.add('has_child_requests')

        yield Record(
            key,
            scenarios_features=list(scenarios_features),
            biometric_features=list(biometric_features),
        )


def get_alice_daily_features(job, job_date):
    last_alice_features_table_date = nd.next_day(job_date, offset=-1, scale='daily')
    alice_features_update_period_start_date = nd.next_day(job_date,
                                                             offset=-ALICE_FEATURES_UPDATE_PERIOD,
                                                             scale='daily')

    old_alice_features = job \
        .table(
            os.path.join(ALICE_FEATURES_PATH, last_alice_features_table_date)) \
        .filter(
            qf.compare('fielddate', '<', alice_features_update_period_start_date))

    new_alice_features = job \
        .table('//home/alice/dialog/prepared_logs_expboxes/{{{start_date}..{end_date}}}'.format(
                start_date=alice_features_update_period_start_date,
                end_date=job_date)) \
        .filter(
            qf.defined('device_id', 'fielddate'),
            qf.compare('fielddate', '>=', alice_features_update_period_start_date),
            qf.one_of('app', ['quasar', 'small_smart_speakers', 'tv'])) \
        .project(
            'fielddate',
            'generic_scenario',
            'child_confidence',

            user_id='device_id') \
        .groupby(
            'user_id', 'fielddate') \
        .reduce(
            alice_daily_features_reducer)

    alice_features = job \
        .concat(
            old_alice_features,
            new_alice_features) \
        .sort('user_id', 'fielddate') \
        .put(
            os.path.join(ALICE_FEATURES_PATH, job_date))

    return alice_features

