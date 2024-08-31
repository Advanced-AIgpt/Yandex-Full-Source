import time
import pytz
from datetime import datetime, timedelta


HOUR = 60 * 60
DAY = 24 * HOUR
WORK_DAY_BEGIN = 10
WORK_DAY_END = 23
LOCAL_TZ = pytz.timezone("Europe/Moscow")


def to_seconds(dt):
    return int(time.mktime(dt.timetuple()))


def calc_business_time_duration(start_timestamp, finish_timestamp, work_day_begin=WORK_DAY_BEGIN, work_date_end=WORK_DAY_END):
    """
    Рассчитывает время между двумя датами с учётом времени рабочего дня
    Т.е. сколько рабочего времени находится между двумя датами
    Рабочее время считается по московскому часовому поясу, а timestamp'ы задаются (всегда) в UTC
    :param int start_timestamp: дата начала в формате unix timestamp
    :param int finish_timestamp: дата окончания в формате unix timestamp
    :param int work_day_begin: начала рабочего дня. По-умолчанию 10
    :param int work_date_end: окончание рабочего дня. По-умолчанию 23
    :return int: длительность в секундах
    """
    if start_timestamp >= finish_timestamp:
        return 0

    def is_overtime(dt):
        if dt.hour < work_day_begin:
            return True
        if dt.hour > work_date_end:
            return True
        if dt.hour == work_date_end and (dt.minute > 0 or dt.second > 0):
            return True
        return False

    work_day_duration = work_date_end - work_day_begin
    start_datetime = datetime.fromtimestamp(start_timestamp, tz=LOCAL_TZ)
    finish_datetime = datetime.fromtimestamp(finish_timestamp, tz=LOCAL_TZ)

    duration_days = (finish_datetime - start_datetime).days
    start_datetime += timedelta(days=duration_days)

    if is_overtime(start_datetime) and is_overtime(finish_datetime):
        return duration_days * work_day_duration * HOUR

    if start_datetime.hour < work_day_begin:
        start_datetime = start_datetime.replace(hour=work_day_begin, minute=0, second=0)

    if start_datetime.hour > work_date_end or (
        start_datetime.hour == work_date_end and (start_datetime.minute > 0 or start_datetime.second > 0)
    ):
        start_datetime = start_datetime.replace(hour=work_day_begin, minute=0, second=0)
        start_datetime += timedelta(days=1)

    if finish_datetime.hour > work_date_end or (
        finish_datetime.hour == work_date_end and (finish_datetime.minute > 0 or finish_datetime.second > 0)
    ):
        finish_datetime = finish_datetime.replace(hour=work_date_end, minute=0, second=0)

    if finish_datetime.hour < work_day_begin:
        finish_datetime = finish_datetime.replace(hour=work_date_end, minute=0, second=0)
        finish_datetime -= timedelta(days=1)

    duration = to_seconds(finish_datetime) - to_seconds(start_datetime)
    if duration < 0:
        duration = 0

    duration += duration_days * work_day_duration * HOUR

    if start_datetime.day < finish_datetime.day:
        duration -= (24 - work_day_duration) * HOUR

    return duration
