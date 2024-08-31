# -*- coding: utf-8 -*-

import os
from datetime import timedelta, datetime
from hashlib import md5
from functools import partial

from nile.api.v1 import (
    datetime as nd,
    extractors as ne,
    with_hints,
    Record,
    cli
)

from qb2.api.v1 import (
    filters as qf,
    typing as qt,
)

IOT_FEATURES_PATH = '${global.iot_features_path}'

DATETIME_FORMAT = '%Y-%m-%dT%H:%M:%S.%fZ'


@with_hints(output_schema=dict(
    puid=qt.String,
    iot_device_id=qt.String,
    iot_device_type=qt.String,
    activation_date=qt.String,
    archivation_date=qt.String,
))
def generate_hardware_device_id(groups, job_date):
    for key, records in groups:
        iot_device_id = md5('.'.join([key.external_id, key.skill_id])).hexdigest()

        activation_dates = set()
        archivation_dates = set()

        for rec in records:
            created = datetime.fromtimestamp(rec.created // 1000000).date().isoformat()

            if rec.archived_at is not None:
                archived_at = datetime.fromtimestamp(rec.archived_at // 1000000).date().isoformat()
            else:
                archived_at = None

            activation_dates.add(created)
            archivation_dates.add(archived_at)

        yield Record(
            puid=str(key.user_id),
            iot_device_id=iot_device_id,
            iot_device_type=key.original_type,
            activation_date=min(activation_dates),
            archivation_date=max(archivation_dates) if None not in archivation_dates else job_date
        )


@with_hints(output_schema=dict(
    iot_device_id=qt.String,
    iot_device_type=qt.String,
    puid=qt.String,
    activation_date=qt.String,
    fielddate=qt.String,
))
def iot_devices_mapper(records):
    for rec in records:
        for fielddate in nd.date_range(rec.activation_date, rec.archivation_date):
            yield Record(
                iot_device_id=rec.iot_device_id,
                iot_device_type=rec.iot_device_type,
                puid=rec.puid,
                activation_date=rec.activation_date,
                fielddate=fielddate
            )


@with_hints(output_schema=dict(
    puid=qt.String,
    fielddate=qt.String,
    iot_active_devices=qt.Yson,
    ))
def iot_devices_reducer(groups):
    for key, records in groups:
        iot_active_devices = set()

        for rec in records:
            iot_device = (rec.iot_device_id, rec.iot_device_type, rec.activation_date)
            iot_active_devices.add(iot_device)

        yield Record(
            key,
            iot_active_devices=list(iot_active_devices)
        )

CLI_OPTIONS = [
    cli.Option('job_date'),
    ]
@cli.statinfra_job(options=CLI_OPTIONS)
def get_iot_daily_features(job, options):
    job_date = options.job_date

    iot_features = job \
        .table('//home/iot/backup/v2/bulbasaur-ydb/{job_date}/Devices'.format(job_date=job_date)) \
        .filter(
            qf.defined('user_id', 'external_id', 'skill_id', 'original_type', 'created'),
            qf.custom(
                lambda original_type:
                'smart_speaker' not in original_type.split('.'),
                'original_type')) \
        .groupby('user_id', 'external_id', 'skill_id', 'original_type') \
        .reduce(
            partial(generate_hardware_device_id, job_date=job_date)) \
        .map(
            iot_devices_mapper,
            intensity='cpu') \
        .groupby(
            'puid',
            'fielddate') \
        .reduce(iot_devices_reducer) \
        .sort('puid', 'fielddate') \
        .put(
            os.path.join(IOT_FEATURES_PATH, job_date),
            ttl=timedelta(days=7))

    return job


if __name__ == '__main__':
    cli.run()
