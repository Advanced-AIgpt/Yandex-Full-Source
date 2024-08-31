# coding: utf-8
from __future__ import absolute_import

import calendar
import pytz

from datetime import datetime as base_datetime


def parse_tz(timezone):
    # Some mobile clients might send invalid timezone codes DIALOG-462
    # If lat=0 and lon=0, geobase returns UTC+0 timezone which is not acceptable by pytz
    if not timezone or timezone == 'UTC+0':
        return pytz.UTC
    fixlist = {
        'GMT': 'Etc/GMT',
        'GMT+00:00': 'Etc/GMT+0',
        'GMT+01:00': 'Etc/GMT+1',
        'GMT+02:00': 'Etc/GMT+2',
        'GMT+03:00': 'Etc/GMT+3',
        'GMT+04:00': 'Etc/GMT+4',
        'GMT+05:00': 'Etc/GMT+5',
        'GMT+05:30': 'Asia/Kolkata',
        'GMT+06:00': 'Etc/GMT+6',
        'GMT+07:00': 'Etc/GMT+7',
        'GMT+08:00': 'Etc/GMT+8',
        'GMT+09:00': 'Etc/GMT+9',
        'GMT+10:00': 'Etc/GMT+10',
        'GMT+11:00': 'Etc/GMT+11',
        'GMT+12:00': 'Etc/GMT+12',
        'GMT-00:00': 'Etc/GMT-0',
        'GMT-01:00': 'Etc/GMT-1',
        'GMT-02:00': 'Etc/GMT-2',
        'GMT-03:00': 'Etc/GMT-3',
        'GMT-04:00': 'Etc/GMT-4',
        'GMT-05:00': 'Etc/GMT-5',
        'GMT-06:00': 'Etc/GMT-6',
        'GMT-07:00': 'Etc/GMT-7',
        'GMT-08:00': 'Etc/GMT-8',
        'GMT-09:00': 'Etc/GMT-9',
        'GMT-10:00': 'Etc/GMT-10',
        'GMT-11:00': 'Etc/GMT-11',
        'GMT-12:00': 'Etc/GMT-12',

        'UTC+01': 'Etc/GMT+1',
        'UTC+02': 'Etc/GMT+2',
        'UTC+03': 'Etc/GMT+3',
        'UTC+04': 'Etc/GMT+4',
        'UTC+05': 'Etc/GMT+5',
        'UTC+06': 'Etc/GMT+6',
        'UTC+07': 'Etc/GMT+7',
        'UTC+08': 'Etc/GMT+8',
        'UTC+09': 'Etc/GMT+9',
        'UTC+10': 'Etc/GMT+10',
        'UTC+11': 'Etc/GMT+11',
        'UTC+12': 'Etc/GMT+12',
        'UTC-0': 'Etc/GMT-0',
        'UTC-01': 'Etc/GMT-1',
        'UTC-02': 'Etc/GMT-2',
        'UTC-03': 'Etc/GMT-3',
        'UTC-04': 'Etc/GMT-4',
        'UTC-05': 'Etc/GMT-5',
        'UTC-06': 'Etc/GMT-6',
        'UTC-07': 'Etc/GMT-7',
        'UTC-08': 'Etc/GMT-8',
        'UTC-09': 'Etc/GMT-9',
        'UTC-10': 'Etc/GMT-10',
        'UTC-11': 'Etc/GMT-11',
        'UTC-12': 'Etc/GMT-12',

        'Africa/Pretoria': 'Africa/Johannesburg',
        'Asia/Hanoi': 'Asia/Bangkok',
        'Asia/Khabarovsk': 'Asia/Vladivostok',
        'Asia/Beijing': 'Asia/Shanghai',
        'Asia/AbuDhabi': 'Asia/Dubai',
        'Asia/Severo-Kurilsk': 'Etc/GMT+11',
        'Etc/GMT+14': 'Etc/GMT-14',
        'Etc/GMT+14:00': 'Etc/GMT-14',
    }
    timezone = fixlist.get(timezone, timezone)
    return pytz.timezone(timezone)


def create_date_safe(year, month, day):
    try:
        return base_datetime(year, month, day)
    except ValueError:
        return None


def utcnow():
    return base_datetime.now(tz=pytz.UTC)


def datetime_to_timestamp(dt):
    if dt.tzinfo:
        # convert to utc
        dt = dt.astimezone(pytz.UTC)

    # else expect naive datetime in utc

    return calendar.timegm(dt.timetuple())


def timestamp_to_datetime(timestamp, tz=pytz.UTC):
    return base_datetime.fromtimestamp(timestamp, tz)


def timestamp_in_ms(dt):
    if dt.tzinfo:
        # convert to utc
        dt = dt.astimezone(pytz.UTC)

    timestamp = datetime_to_timestamp(dt)
    return timestamp * 1000 + int(round(dt.microsecond / 1000))


def get_tz_name(dt):
    return (dt.tzinfo and dt.tzinfo.zone) or 'UTC'
