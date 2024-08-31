# coding: utf-8

from __future__ import division
from builtins import str
from past.utils import old_div
from datetime import datetime, timedelta
import pytz


import sys
reload(sys)  # noqa
sys.setdefaultencoding('utf8')  # noqa

from .utils import load_translations
t = load_translations('visualize_alarms_timers')
t.install()


def _(text):
    """
    HACK: Обёртка над gettext для python2 строк
    вместо: `_ = t.gettext`
    :param str text:
    """
    return t.gettext(text.decode('utf-8'))


DAYS = [
    ('MO', _('понедельникам')),
    ('TU', _('вторникам')),
    ('WE', _('средам')),
    ('TH', _('четвергам')),
    ('FR', _('пятницам')),
    ('SA', _('субботам')),
    ('SU', _('воскресеньям')),
]

ALARM_REMINDER_PREFIX = _('Срабатывает напоминание: ')


def _get_cur_date(record):
    """
    Возвращает отображаемое время
    Используется серверное время прокачки или клиентское время устройства
    :param dict record:
    :return str:
    """
    cur_timestamp = record.get('ts') or record['client_time']
    return datetime \
        .utcfromtimestamp(cur_timestamp + get_time_offset(_get_tz_from_record(record))) \
        .strftime('%Y-%m-%d %H:%M:%S')


def get_time_offset(client_tz=None):
    """определяет смещение таймзоны в минутах
    :param client_tz: строка с временной зоной в виде continent/city или +0X:00
    При нулевой или пустой таймзоне предполагает московскую, что лучше, чем UTC,
    но желательно починить определение таймзоны в логах."""
    client_tz = client_tz or "+03"
    try:
        client_tz = datetime.now(pytz.timezone(client_tz)).strftime('%z')
    except pytz.exceptions.UnknownTimeZoneError:
        pass
    if client_tz[0] in ('+', '-'):
        if len(client_tz) == 3:
            offset = 3600 * int(client_tz)
        else:
            offset = 3600 * int(client_tz[:-2]) + 60 * int(client_tz[0] + client_tz[-2:])
    else:
        offset = 10800
    return offset


def alarms_processing(ics_format_string, tz, cur_date, currently_playing=False):
    DATE_FORMAT = '%Y-%m-%d %H:%M:%S'
    all_alarms, true_alarms = [], []
    cur_alarm = []

    for i in ics_format_string.split('\r\n'):
        if i == 'BEGIN:VEVENT':
            cur_alarm = []
        elif i == 'END:VEVENT':
            alarm_new_datetime = (datetime.strptime(cur_alarm[0][8:], '%Y%m%dT%H%M%SZ') +
                                  timedelta(seconds=get_time_offset(tz))).strftime(DATE_FORMAT)

            if cur_alarm[2].startswith('RRULE:FREQ=WEEKLY;BYDAY='):
                alarm_text = _('. Будильник начиная с {} повторяется еженедельно по ').format(alarm_new_datetime) + \
                    ', '.join([y for y in [x[1] if x[0] in cur_alarm[2][24:] else None for x in DAYS] if y])
                all_alarms.append((alarm_text, alarm_new_datetime, 1))
            else:
                alarm_text = _('. Будильник на ') + alarm_new_datetime
                all_alarms.append((alarm_text, alarm_new_datetime, 0))
        else:
            cur_alarm.append(i)

    for (alarm_text, alarm_datetime, is_alarm_repeats) in sorted(all_alarms, key=lambda x: x[1], reverse=True):
        if alarm_datetime >= cur_date or is_alarm_repeats:
            true_alarms.append(alarm_text)
        elif currently_playing:
            dt = datetime.strptime(alarm_datetime, DATE_FORMAT) - datetime.strptime(cur_date, DATE_FORMAT)
            if timedelta(hours=-2) < dt < timedelta(hours=2):
                true_alarms.append(alarm_text)
                break

    return '\n'.join([
        str(idx + 1) + alarm_text
        for idx, alarm_text in enumerate(true_alarms)
    ])


def _get_tz_from_record(record, default_tz="Europe/Moscow"):
    """select client TZ value from record  - checks fields `tz` and `client_tz`
    TO DO: consider guessing TZ based on location info if present."""
    return record.get('tz') or record.get('client_tz') or default_tz


def get_time_str(timestamp, on=False, when=False):
    aggr_str = ''
    if timestamp >= 60:
        aggr_str += str(old_div(timestamp, 60))
        if on:
            aggr_str += _(' минуте')
        elif when:
            aggr_str += _(' минуту')
        elif (old_div(timestamp, 60)) % 10 == 1:
            aggr_str += _(' минута')
        elif ((old_div(timestamp, 60)) % 10 in (2, 3, 4)) and ((old_div(timestamp, 60)) % 100 not in (12, 13, 14)):
            aggr_str += _(' минуты')
        else:
            aggr_str += _(' минут')
    if timestamp == 0:
        if on:
            aggr_str += _('самом начале')
        elif when:
            aggr_str += _('самое начало')
        else:
            aggr_str += _('0 секунд')
    if timestamp % 60 != 0:
        if old_div(timestamp, 60) != 0:
            aggr_str += ' '
        aggr_str += str(timestamp % 60)
        if on:
            aggr_str += _(' секунде')
        elif when:
            aggr_str += _(' секунду')
        elif (timestamp % 60) % 10 == 1:
            aggr_str += _(' секунда')
        elif ((timestamp % 60) % 10 in (2, 3, 4)) and ((timestamp % 60) % 100 not in (12, 13, 14)):
            aggr_str += _(' секунды')
        else:
            aggr_str += _(' секунд')
    return aggr_str


def timers_processing(active_timers):
    timers_st = ''
    add_st = ''
    for idx_timer, timer in enumerate(active_timers):
        if len(active_timers) > 1:
            timers_st += str(idx_timer + 1) + '. '
        timers_st += _('Таймер на ') + get_time_str(timer['duration']) + _(', осталось ') + \
                     get_time_str(timer['remaining']) + '. '
        if timer['paused']:
            if len(active_timers) > 1:
                add_st += _('Таймер') + str(idx_timer + 1) + _('поставлен на паузу.')
            else:
                add_st += _('Таймер поставлен на паузу.')
        if timer['currently_playing']:
            if len(active_timers) > 1:
                add_st += _('Таймер') + str(idx_timer + 1) + _('истек и сейчас звучит.')
            else:
                add_st += _('Таймер истек и сейчас звучит.')
        if idx_timer < len(active_timers) - 1:
            timers_st += '\n'
    ret = {
        'type': _('Активные таймеры'),
        'content': timers_st
    }
    if add_st != '':
        ret['playback'] = add_st
    return ret
