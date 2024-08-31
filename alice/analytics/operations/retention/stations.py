# -*- coding: utf-8 -*-

from nile.api.v1 import (
    aggregators as na,
    extractors as ne,
    datetime as nd,
    with_hints,
    Record,
)

from qb2.api.v1 import (
    filters as qf,
    typing as qt,
    extractors as qe,
)

from alice.analytics.operations.retention.features.subscriptions import get_subscription_daily_features
from alice.analytics.operations.retention.features.alice_features import get_alice_daily_features
from alice.analytics.operations.retention.features.iot_features import get_iot_daily_features


@with_hints(output_schema=dict(
    user_id=qt.String,
    fielddate=qt.String,
    activation_date=qt.String,
    device_type=qt.Optional[qt.String],
    region_id=qt.Optional[qt.Int64],
    country_id=qt.Optional[qt.Int64],
    hdmi_plugged=qt.Optional[qt.Bool],
    puid=qt.Optional[qt.String],
    subscription_state=qt.Optional[qt.String],
    subscription_declared_state=qt.Optional[qt.String],
    scenarios_features=qt.Yson,
    has_child_requests=qt.Bool,
    iot_active_devices=qt.Yson,
))
def stations_dayuse_features_mapper(records):
    for rec in records:
        has_child_requests = False

        if rec.biometric_features and 'has_child_requests' in rec.biometric_features:
            has_child_requests = True

        yield Record(
            user_id=rec.user_id,
            fielddate=rec.fielddate,
            activation_date=rec.activation_date,
            device_type=rec.device_type,
            region_id=rec.region_id,
            country_id=rec.country_id,
            hdmi_plugged=rec.hdmi_plugged,
            puid=rec.puid,

            subscription_state=rec.state,
            subscription_declared_state=rec.declared_state,

            scenarios_features=rec.scenarios_features or [],
            has_child_requests=has_child_requests,

            iot_active_devices=rec.iot_active_devices or [],
        )


@with_hints(output_schema=dict(
    user_id=qt.String,
    activation_features=qt.Yson,
    activation_scenarios_features=qt.Yson
))
def stations_activation_features_reducer(groups):
    for key, records in groups:
        activation_features = set()
        activation_scenarios_features = set()
        has_any_subscription = False

        for rec in records:
            if nd.distance_in_periods(rec.activation_date, rec.fielddate) < 7:
                if rec.puid is not None:
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

                if rec.hdmi_plugged:
                    activation_features.add('hdmi_plugged_7d')

                if rec.has_child_requests:
                    activation_features.add('has_child_requests_7d')

                for scenario in rec.scenarios_features:
                    activation_scenarios_features.add(scenario)

                for iot_device in rec.iot_active_devices:
                    activation_features.add('has_iot_active_device_7d')

                    if rec.activation_date <= iot_device[2] <= rec.fielddate:
                        activation_features.add('has_iot_device_activation_7d')

        if not has_any_subscription:
            activation_features.add('no_any_subscription_7d')

        yield Record(
            key,
            activation_features=list(activation_features),
            activation_scenarios_features=list(activation_scenarios_features)
        )


def get_stations_retention_cube(job, job_date):
    subscription_features = get_subscription_daily_features(job, job_date)
    alice_features = get_alice_daily_features(job, job_date)
    iot_features = get_iot_daily_features(job, job_date)

    dayuse = job \
        .table('//home/sda/cubes/station/dayuse/{}'.format(job_date)) \
        .filter(
            qf.defined('device_id', 'fielddate'),
            qf.not_(qf.startswith('device_type', 'yandex_tv')),
            qf.not_(qf.one_of('device_type', {'elariwatch', 'elari÷atch'})),
            ) \
        .project(
            'fielddate',
            'device_type',
            'puid',
            'hdmi_plugged',
            'geo_id',
            user_id='device_id') \
        .unique('user_id', 'fielddate')

    activations = dayuse \
        .groupby('user_id') \
        .aggregate(
            activation_date=na.min('fielddate'),
            device_type=na.first(
                'device_type',
                by='fielddate',
                predicate=qf.defined('device_type')),
            geo_id=na.first(
                'geo_id',
                by='fielddate',
                predicate=qf.defined('geo_id'))
            ) \
        .project(ne.all(exclude=['geo_id']),
                 region_id='geo_id',
                 country_id=qe.yql_custom(
                     'country_id', 'Geo::RoundRegionById(cast($p0 as int32), \"country\").id', 'geo_id'),
                 )

    dayuse_with_activations_and_features = dayuse \
        .project(
            ne.all(exclude=['device_type'])) \
        .join(
            activations,
            by='user_id',
            assume_unique_right=True) \
        .join(
            subscription_features,
            by=['puid', 'fielddate'],
            type='left',
            assume_unique_right=True) \
        .join(
            alice_features,
            by=['user_id', 'fielddate'],
            type='left',
            assume_unique_right=True) \
        .join(
            iot_features,
            by=['puid', 'fielddate'],
            type='left',
            assume_unique_right=True) \
        .map(stations_dayuse_features_mapper)

    activations_features = dayuse_with_activations_and_features \
        .groupby('user_id') \
        .reduce(stations_activation_features_reducer)

    retention_cube = dayuse_with_activations_and_features \
        .project(
            ne.all(exclude=['iot_active_devices'])) \
        .join(
            activations_features,
            by='user_id',
            assume_unique_right=True) \
        .sort('activation_date', 'device_type', 'user_id', 'fielddate')

    return retention_cube
