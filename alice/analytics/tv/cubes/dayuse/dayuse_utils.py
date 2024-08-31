from datetime import (
    datetime,
    timedelta,
)
from constants import (
    DATETIME_FORMAT,
    DATE_FORMAT,
)


def get_counter_most_common_element(counter, default=None):
    if counter.most_common():
        return counter.most_common()[0][0]
    return default


def get_set_not_null_elements(set_items):
    return [item for item in set_items if item is not None]


def get_device_identifier(rec):
    device_name = rec['hdmi_session_device_name'] if 'hdmi_session_device_name' in rec else 'undefined_device'
    port = rec['hdmi_session_port'] if 'hdmi_session_port' in rec else 'undefined_port'
    return device_name, port


def to_datetime(datetime_str):
    return datetime.strptime(datetime_str, DATETIME_FORMAT)


def seconds_between(first_datetime_str, second_datetime_str=None, next_day=False):
    first_event_datetime = to_datetime(first_datetime_str)
    if second_datetime_str:
        second_datetime = to_datetime(second_datetime_str)
    else:
        second_datetime = datetime(
            first_event_datetime.year,
            first_event_datetime.month,
            first_event_datetime.day + (1 if next_day else 0)
        )
    return abs((first_event_datetime - second_datetime).total_seconds())


def dict_to_generator(d):
    if isinstance(d, dict):
        for k in d:
            for v in dict_to_list(d[k]):
                yield (k,) + v
    elif isinstance(d, list) or isinstance(d, tuple):
        for v in d:
            yield (v,)
    else:
        yield (d,)


def dict_to_list(d):
    return list(dict_to_generator(d))


def get_date_clamp_time(fielddate, start_datetime, end_datetime):
    if fielddate is None or start_datetime is None or end_datetime is None:
        return 0

    fielddate = datetime.strptime(fielddate, DATE_FORMAT)
    next_day = fielddate + timedelta(days=1)
    start_datetime = min(datetime.strptime(start_datetime, DATETIME_FORMAT), next_day)
    end_datetime = max(datetime.strptime(end_datetime, DATETIME_FORMAT), fielddate)

    fielddate_start = max(start_datetime, fielddate)
    fielddate_end = min(end_datetime, next_day)

    return (fielddate_end - fielddate_start).seconds


def increment_path(data, path, value):
    if len(path) == 1:
        data.setdefault(path[0], 0)
        data[path[0]] += value
    elif len(path) > 1:
        data.setdefault(path[0], {})
        increment_path(data[path[0]], path[1:], value)
    return data


def get_paths_and_values(cur_data):
    result = []
    for key in cur_data.keys():
        if isinstance(cur_data[key], dict):
            result.extend([([key] + path, value) for path, value in get_paths_and_values(cur_data[key])])
        else:
            result.append(([key], cur_data[key]))
    return result


def str_to_date(str_date):
    if str_date is None:
        return None
    return datetime.strptime(str_date, DATE_FORMAT)


def is_new(activation_date, fielddate, max_days_diff=7):
    if activation_date is None or fielddate is None:
        return None
    delta_time = str_to_date(fielddate) - str_to_date(activation_date)
    return delta_time.days <= max_days_diff


def merge_info(data, midterm_data, value_func=lambda x: min(1, x)):
    for path, value in get_paths_and_values(midterm_data):
        data = increment_path(data, path, value_func(value))
    return data


def get_hdmi_device_info(event_value):
    return {
        'session_device_name': event_value.get('device_name', 'no_device'),
        'session_port': event_value.get('port', 'no_port')
    }


def get_hdmi_device_name(event_value):
    return {
        'session_device_name': event_value.get('device_name', 'no_device'),
        'session_port': event_value.get('port', 'no_port')
    }
