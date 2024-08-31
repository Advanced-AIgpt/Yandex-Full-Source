import os
import time
from datetime import date, timedelta
from collections import Counter
import ast

from nile.api.v1 import (
    extractors as ne,
    aggregators as na,
    datetime as nd,
    files as nfl,
    clusters,
    cli,
    with_hints,
    Record)

from qb2.api.v1 import (
    typing as qt,
    filters as qf,
    extractors as qe)

from utils.nirvana.op_caller import call_as_operation


# FROM https://a.yandex-team.ru/arc/trunk/arcadia/statbox/jam/libs/sda/utils/common.py

def list_zip_to_dict(keys, values, load_json=True):
    if not (keys and values):
        return dict()
    try:
        if load_json:
            a, b = ast.literal_eval(keys), ast.literal_eval(values)
        else:
            a, b = keys, values
        return dict(zip(a, b))
    except:
        return dict()


def get_value_by_key(keys, values, key, default=None, load_json=True, convert_type=str):
    try:
        zipped_dict = list_zip_to_dict(keys, values, load_json)
        if zipped_dict.get(key):
            if convert_type:
                return convert_type(zipped_dict.get(key))
            return zipped_dict.get(key)
        return default
    except:
        return default


def mapper_wrapper(cls):
    def wrapped(*args, **kwargs):
        instance = cls(*args, **kwargs)
        return with_hints(instance.output_schema)(instance)

    return wrapped


def get_counter_most_common_element(counter, default=None):
    if counter.most_common():
        return counter.most_common()[0][0]
    return default


# FROM https://a.yandex-team.ru/arc/trunk/arcadia/statbox/jam/jobs/sda/cubes/station/dayuse/utils.py

def get_device_type(device_type, model):
    if device_type == 'desktop':
        return model
    elif device_type == "Station" and model == "lightcomm":  # update
        return "lightcomm"
    elif device_type:
        return device_type
    elif device_type == '' and model == 'Station':
        return 'yandexstation'
    elif device_type == '' and model == 'YandexModule-00002':
        return 'yandexmodule'
    elif device_type == '' and model == 'KidPhone3G':
        return 'elariwatch'
    else:
        return model


def is_hdmi_plugged(hdmi_plugged):
    if hdmi_plugged is not None:
        if hdmi_plugged == '1':
            return True
        return False
    return None


# FROM https://a.yandex-team.ru/arc/trunk/arcadia/statbox/jam/jobs/sda/cubes/station/dayuse/dayuse_reducer.py

@mapper_wrapper
class DayuseReducer(object):
    output_schema = dict(
        device_id=qt.Optional[qt.String],
        fielddate=qt.Optional[qt.String],
        device_type=qt.Optional[qt.String],
        uuid=qt.Optional[qt.String],
        puid=qt.Optional[qt.String],
        app_version=qt.Optional[qt.String],
        devprod=qt.Optional[qt.String],
        manufacturer=qt.Optional[qt.String],
        model=qt.Optional[qt.String],
        hdmi_plugged=qt.Optional[qt.Bool],
        geo_id=qt.Optional[qt.Int64],
    )

    def __call__(self, groups):
        for key, records in groups:
            results = {}

            counter_fields = {
                'uuid': Counter(),
                'puid': Counter(),
                'hdmi_plugged': Counter(),
                'manufacturer': Counter(),
                'model': Counter(),
                'geo_id': Counter(),
                'device_type': Counter(),
            }

            last_timestamp_fields = {
                'app_version': None,
                'devprod': None,
            }

            other_fields = {
                'hdmi_plugged': set()
            }

            for rec in records:
                for field in counter_fields:
                    if rec[field] is not None:
                        counter_fields[field][rec[field]] += 1

                for field in last_timestamp_fields:
                    if rec[field] is not None:
                        last_timestamp_fields[field] = rec[field]

                if rec['hdmi_plugged'] is not None:
                    other_fields['hdmi_plugged'].add(is_hdmi_plugged(rec['hdmi_plugged']))

            for field in counter_fields:
                results[field] = get_counter_most_common_element(counter_fields[field])

            for field in last_timestamp_fields:
                results[field] = last_timestamp_fields[field]

            if len(other_fields['hdmi_plugged']) > 0:
                results['hdmi_plugged'] = max(other_fields['hdmi_plugged'])
            else:
                results['hdmi_plugged'] = None

            yield Record(key, **results)


def make_job(job, job_date, update_period, output_table_path, last_dayuse_table_path):
    update_period_start_date = nd.next_day(job_date, offset=-update_period, scale='daily')

    subscription_devices = job \
        .table(SUBSCRIPTION_DEVICES_PATH) \
        .project(
            'device_id',
            is_subscription_device=ne.const(True).with_type(qt.Bool)) \
        .unique('device_id')

    appmetrica_log = job \
        .table(
            METRIKA_MOBILE_LOG_PATH.format(
                start_date=update_period_start_date, end_date=job_date)) \
        .qb2(
            log='metrika-mobile-log',

            filters=[
                qf.equals('api_key', STATION_APIKEY),
                qf.one_of('event_type', [2, 4]),
                qf.compare('event_date', '>=', update_period_start_date),
                qf.compare('event_date', '<=', job_date)
            ],

            fields=[
                'uuid', 'puid', 'app_version',
                'event_timestamp', 'geo_id', 'event_date', 'device_id',
                qe.log_field('APIKey').allow_override().rename('api_key').with_type(qt.UInt32).hide(),
                qe.log_field('EventType').allow_override().rename('event_type').with_type(qt.UInt32).hide(),
                qe.log_field('DeviceID').allow_override().rename('device_id'),
                qe.log_field('Manufacturer').rename('manufacturer'),
                qe.log_field('Model').rename('model'),
                qe.log_field('ReportEnvironment_Keys').rename('report_environment_keys').with_type(qt.List[qt.String]),
                qe.log_field('ReportEnvironment_Values').rename('report_environment_values').with_type(qt.List[qt.String]),
            ]) \
        .project(
            ne.all(exclude=['report_environment_keys', 'report_environment_values', 'event_date', 'device_id']),

            _device_type=ne.custom(
                lambda x, y: get_value_by_key(x, y, key='device_type', load_json=False),
                'report_environment_keys', 'report_environment_values'
                ).with_type(qt.Optional[qt.String]).hide(),

            _quasmodrom_group=ne.custom(
                lambda x, y: get_value_by_key(x, y, key='quasmodrom_group', load_json=False),
                'report_environment_keys', 'report_environment_values'
            ).with_type(qt.Optional[qt.String]).hide(),

            _build_type=ne.custom(
                lambda x, y: get_value_by_key(x, y, key='buildType', load_json=False),
                'report_environment_keys', 'report_environment_values'
            ).with_type(qt.Optional[qt.String]).hide(),

            device_id=ne.custom(
                lambda x, y, z: get_value_by_key(x, y, key='device_id', load_json=False) or z,
                'report_environment_keys', 'report_environment_values', 'device_id'
            ).with_type(qt.Optional[qt.String]),

            hdmi_plugged=ne.custom(
                lambda x, y: get_value_by_key(x, y, key='hdmiPlugged', load_json=False),
                'report_environment_keys', 'report_environment_values'
            ).with_type(qt.Optional[qt.String]),

            device_type=ne.custom(
                lambda x, y: get_device_type(x, y) or x,
                '_device_type', 'model'
            ).with_type(qt.Optional[qt.String]),

            devprod=ne.custom(
                lambda quasmodrom_group, build_type:
                quasmodrom_group or build_type or '-',
                '_quasmodrom_group', '_build_type'
            ).with_type(qt.Optional[qt.String]),

            fielddate='event_date'
        )

    dayuse_cube_new = appmetrica_log \
        .groupby('device_id', 'fielddate') \
        .sort('event_timestamp') \
        .reduce(DayuseReducer()) \
        .join(
            subscription_devices,
            by='device_id',
            type='left',
            assume_unique_right=True) \
        .project(
            ne.all(exclude=['is_subscription_device', 'device_type']),

            is_subscription_device=ne.custom(
                lambda is_subscription_device: is_subscription_device or False,
                'is_subscription_device'
                ).with_type(qt.Bool),

            device_type=ne.custom(
                lambda device_type, is_subscription_device:
                '_'.join([device_type, 'subscription']) if device_type is not None and is_subscription_device else device_type
                ).with_type(qt.Optional[qt.String]))

    dayuse_cube_old = job \
        .table(last_dayuse_table_path) \
        .filter(qf.compare('fielddate', '<', update_period_start_date))

    dayuse_cube = job \
        .concat(dayuse_cube_old, dayuse_cube_new) \
        .sort('fielddate', 'device_type', 'device_id') \
        .put(output_table_path,
             ttl=timedelta(77))

    return job, dayuse_cube


METRIKA_MOBILE_LOG_PATH = '//logs/appmetrica-yandex-events/1d/{{{start_date}..{end_date}}}'
SUBSCRIPTION_DEVICES_PATH = '//home/quasar-dev/backend/snapshots/current/device_subscription'
STATIONS_DAYUSE_CUBE_DIR = '//home/sda/cubes/station/dayuse'
STATIONS_DAYUSE_CUBE_DIR_BETA = '//home/sda/cubes/station/dayuse_beta'  # temporary test dir

STATION_APIKEY = 999537
DEFAULT_UPDATE_PERIOD = 7


def main(job_date=None, update_period=None, last_dayuse_table_path=None):
    job_date = job_date or "2021-06-21"
    update_period = update_period or DEFAULT_UPDATE_PERIOD
    cluster = clusters.Hahn()
    job = cluster.job()

    last_dayuse_table_date = nd.next_day(job_date, offset=-1, scale='daily')
    if last_dayuse_table_path is None:
        last_dayuse_table_path = os.path.join(STATIONS_DAYUSE_CUBE_DIR, last_dayuse_table_date)
    output_table_path = os.path.join(STATIONS_DAYUSE_CUBE_DIR_BETA, job_date)
    job, cube = make_job(job, job_date, update_period, output_table_path, last_dayuse_table_path)
    job.run()
    return {"cluster": "hahn", "table": output_table_path}


if __name__ == '__main__':
    st = time.time()
    print('start at', time.ctime(st))
    call_as_operation(main)
    print('total elapsed {:2f} min'.format((time.time() - st) / 60))
