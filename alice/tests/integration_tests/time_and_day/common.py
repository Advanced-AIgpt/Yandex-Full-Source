import pytz


Months = [
    'января', 'февраля',
    'марта', 'апреля', 'мая',
    'июня', 'июля', 'августа',
    'сентября', 'октября', 'ноября',
    'декабря',
]
MonthsIndex = {m: i + 1 for i, m in enumerate(Months)}


Weekdays = [
    'понедельник', 'вторник', 'среда', 'четверг', 'пятница', 'суббота', 'воскресенье',
]
WeekdaysIndex = {d: i for i, d in enumerate(Weekdays)}


class Date(object):
    def __init__(self, weekday, month, day):
        self.weekday = WeekdaysIndex[weekday.lower()]
        self.month = MonthsIndex[month.lower()] if month else None
        self.day = int(day) if day else None

    def __eq__(self, other):
        return self.day == other.day and self.month == other.month and self.weekday == other.weekday()

    def __repr__(self):
        return f'Date(weekday={self.weekday}, month={self.month}, day={self.day})'


class Time(object):
    def __init__(self, hour, minute):
        self.hour = int(hour)
        self.minute = int(minute or 0)

    def __eq__(self, other):
        return self.hour == other.hour and self.minute == other.minute

    def __repr__(self):
        return f'Time(hour={self.hour}, minute={self.minute})'


class DateTime(object):
    def __init__(self, weekday, month, day, hour, minute):
        self._date = Date(weekday, month, day)
        self._time = Time(hour, minute)

    def __getattr__(self, attr):
        return getattr(self._date, attr, None) or getattr(self._time, attr, None)

    def __eq__(self, other):
        return self._date == other and self._time == other


class DateTimeRange(object):
    def __init__(self, before, after, timezone=None):
        tz = pytz.timezone(timezone) if timezone else None
        self.before = before.astimezone(tz) if tz else before
        self.after = after.astimezone(tz) if tz else after

    def __contains__(self, other):
        return self.before == other or self.after == other
