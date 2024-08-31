# -*- coding: utf-8 -*-

import os

from qb2.api.v1 import (
    filters as qf
)


def get_subscription_daily_features(job, job_date):
    subscription_states = job \
        .table(os.path.join('//home/sda/cubes/common/subscriptions', job_date)) \
        .filter(
            qf.nonzero('plus')) \
        .project(
            'puid',
            'fielddate',
            'state',
            'declared_state')

    return subscription_states
