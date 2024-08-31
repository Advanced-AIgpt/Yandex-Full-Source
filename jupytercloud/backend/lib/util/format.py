import calendar
import datetime
import json


def pretty_float(v, precision=3):
    """
     Format various numbers into pretty float strings.

    >>> pretty_float(99.1235)
    99.124
    >>> pretty_float(100.0001235)
    100.000124
    >>> pretty_float(0.000123456, 5)
    0.00012346
    >>> pretty_float(-0.000000123456, 4)
    -1.235e-07
    >>> pretty_float(-1.00123456, 4)
    -1.001235
    >>> pretty_float(0)
    0
    >>> pretty_float(None)
    >>> pretty_float('99.1235')
    99.124
    """
    if v is None:
        return None

    v = float(v)

    if int(v) == v:
        return int(v)

    if not v:
        return v

    fractional = abs(v)
    if fractional >= 1:
        fractional = fractional % int(fractional)

    n = 0
    while fractional < 1 and n < 20:
        n += 1
        fractional *= 10

    return round(float(v), n + precision - 1)


def json_default_factory(datetime_serializer=lambda dt: calendar.timegm(dt.timetuple())):
    def default(value):
        if isinstance(value, datetime.datetime):
            return datetime_serializer(value)

        if callable(value):
            return repr(value)

        raise TypeError(f"can't serialize value of type {type(value)}: {value}")

    return default


def pretty_json(
    obj,
    *,
    ensure_ascii=False,
    sort_keys=True,
    indent=2,
    skipkeys=True,
    default=json_default_factory(),
):
    """Like pformat, but json.
    json.dumps with sane settings for logging"""

    return json.dumps(
        obj,
        ensure_ascii=ensure_ascii,
        sort_keys=sort_keys,
        indent=indent,
        skipkeys=skipkeys,
        default=default,
    )
