# -*- coding: utf-8 -*-

from nile.api.v1 import (
    aggregators as na,
    extractors as ne,
    datetime as nd,
    with_hints,
    Record,
)

from qb2.api.v1 import (
    extractors as qe,
    filters as qf,
    typing as qt,
)

from alice.analytics.operations.retention.features.subscriptions import get_subscription_daily_features


@with_hints(output_schema=dict(
    user_id=qt.String,
    fielddate=qt.String,
    is_logged_in=qt.Bool,
    puid=qt.Optional[qt.String],
    activation_date=qt.String,
    device_type=qt.Optional[qt.String],
    manufacturer=qt.Optional[qt.String],
    platform=qt.Optional[qt.String],
    diagonal=qt.Optional[qt.Float],
    resolution=qt.Optional[qt.String],
    subscription_state=qt.Optional[qt.String],
    subscription_declared_state=qt.Optional[qt.String],
))
def tvandroid_dayuse_features_mapper(records):
    for rec in records:
        is_logged_in = False

        if rec.puid is not None:
            is_logged_in = True

        yield Record(
            user_id=rec.user_id,
            fielddate=rec.fielddate,
            is_logged_in=is_logged_in,
            puid=rec.puid,
            activation_date=rec.activation_date,
            device_type=rec.device_type,
            manufacturer=rec.manufacturer,
            platform=rec.platform,
            diagonal=rec.diagonal,
            resolution=rec.resolution,
            subscription_state=rec.state,
            subscription_declared_state=rec.declared_state
        )


@with_hints(output_schema=dict(
    user_id=qt.String,
    activation_features=qt.Yson,
))
def tvandroid_activation_features_reducer(groups):
    for key, records in groups:
        activation_features = set()
        has_any_subscription = False

        for rec in records:
            if nd.distance_in_periods(rec.activation_date, rec.fielddate) < 7:
                if rec.puid is not None:
                    activation_features.add('is_logged_in_7d')

                    if rec.subscription_state is not None:
                        has_any_subscription = True

                    if rec.subscription_state == 'active':
                        activation_features.add('has_active_subscription_7d')

                        if rec.subscription_declared_state == 'trial':
                            activation_features.add('has_trial_subscription_7d')
                        elif rec.subscription_declared_state == 'transactional':
                            activation_features.add('has_promo_subscription_7d')
                        elif rec.subscription_declared_state == 'premium':
                            activation_features.add('has_premium_subscription_7d')
                    elif rec.subscription_state == 'churned':
                        activation_features.add('has_churned_subscription_7d')

        if not has_any_subscription:
            activation_features.add('no_any_subscription_7d')

        yield Record(
            key,
            activation_features=list(activation_features)
        )


def get_tvandroid_retention_cube(job, job_date):
    subscription_features = get_subscription_daily_features(job, job_date)

    dayuse = job \
        .table('//home/sda/cubes/tv/dayuse/{}'.format(job_date)) \
        .filter(
            qf.defined('fielddate'),
            qf.or_(qf.defined('eth0'), qf.defined('quasar_device_id'))) \
        .project(
            'fielddate',
            'manufacturer',
            'platform',
            'diagonal',
            'resolution',
            'is_logged_in',
            'puid',
            qe.or_('user_id', 'eth0', 'quasar_device_id').with_type(qt.String)) \
        .unique('user_id', 'fielddate')


    activations = dayuse \
        .groupby('user_id') \
        .aggregate(
            activation_date=na.min('fielddate'),

            manufacturer=na.first('manufacturer', by='fielddate'),
            platform=na.first('platform', by='fielddate'),
            diagonal=na.first('diagonal', by='fielddate'),
            resolution=na.first('resolution', by='fielddate')) \
        .project(
            ne.all(),
            device_type=ne.const('tvandroid').with_type(qt.String))


    dayuse_with_activations_and_features = dayuse \
        .project(
            ne.all(exclude=['manufacturer', 'platform', 'diagonal', 'resolution'])) \
        .join(
            activations,
            by='user_id',
            assume_unique_right=True) \
        .join(
            subscription_features,
            by=['puid', 'fielddate'],
            type='left',
            assume_unique_right=True) \
        .map(tvandroid_dayuse_features_mapper)


    activations_features = dayuse_with_activations_and_features \
        .groupby('user_id') \
        .reduce(tvandroid_activation_features_reducer)


    retention_cube = dayuse_with_activations_and_features \
        .join(
            activations_features,
            by='user_id',
            assume_unique_right=True) \
        .sort('activation_date', 'device_type', 'user_id', 'fielddate')

    return retention_cube
