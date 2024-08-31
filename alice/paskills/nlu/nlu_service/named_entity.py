# coding: utf-8

from __future__ import unicode_literals

import logging

import attr

from nlu_service.services import unistat

# This file defines named entities available in external skills public API
# to_dict() and get_serialized_value() changes should be backwards compatible

logger = logging.getLogger(__name__)


@attr.s(cmp=False)
class NamedEntity(object):

    type = None

    start_token = attr.ib(default=None)
    end_token = attr.ib(default=None)

    def get_serialized_value(self):
        # should return JSON-serializable value
        raise NotImplementedError()

    def is_valid(self):
        serialized_value = self.get_serialized_value()
        is_valid = (
            serialized_value is not None
            and serialized_value != {}
        )
        return (
            serialized_value is not None
            and serialized_value != {}
        )

    def to_dict(self):
        return {
            'type': self.type,
            'tokens': {
                'start': self.start_token,
                'end': self.end_token,
            },
            'value': self.get_serialized_value(),
        }

    def __gt__(self, other):
        return (self.start_token, self.end_token) > (other.start_token, other.end_token)

    def __lt__(self, other):
        return (self.start_token, self.end_token) < (other.start_token, other.end_token)

    def __eq__(self, other):
        return (
            isinstance(other, self.__class__) and
            self.start_token == other.start_token and
            self.end_token == other.end_token and
            self.get_serialized_value() == other.get_serialized_value()
        )


@attr.s(cmp=False)
class NumberEntity(NamedEntity):

    type = 'YANDEX.NUMBER'

    value = attr.ib(default=None)

    def get_serialized_value(self):
        return self.value


@attr.s(cmp=False)
class DateTimeEntity(NamedEntity):

    type = 'YANDEX.DATETIME'

    year = attr.ib(default=None)
    year_is_relative = attr.ib(default=False)
    month = attr.ib(default=None)
    month_is_relative = attr.ib(default=False)
    day = attr.ib(default=None)
    day_is_relative = attr.ib(default=False)
    hour = attr.ib(default=None)
    hour_is_relative = attr.ib(default=False)
    minute = attr.ib(default=None)
    minute_is_relative = attr.ib(default=False)

    @classmethod
    def _parse_date_fields(cls, wizard_entry):
        """Translate wizard-style datetime fields to DateTimeEntity attributes"""
        return {
            'year': wizard_entry.get('Year'),
            'year_is_relative': wizard_entry.get('RelativeYear'),
            'month': wizard_entry.get('Month'),
            'month_is_relative': wizard_entry.get('RelativeMonth'),
            'day': wizard_entry.get('Day'),
            'day_is_relative': wizard_entry.get('RelativeDay'),
            'hour': wizard_entry.get('Hour'),
            'hour_is_relative': wizard_entry.get('RelativeHour'),
            'minute': wizard_entry.get('Min'),
            'minute_is_relative': wizard_entry.get('RelativeMin'),
            'week': wizard_entry.get('Week'),
            'week_is_relative': wizard_entry.get('RelativeWeek'),
        }

    @classmethod
    def _parse_duration(cls, duration):
        type = duration['Type']
        sign = -1 if type == 'BACK' else 1
        kwargs = {
            key: sign * value
            for key, value in cls._parse_date_fields(duration).iteritems()
            if value is not None
        }
        if 'week' in kwargs:
            kwargs['day'] = kwargs.get('day', 0) + kwargs['week'] * 7

        kwargs.pop('week', None)
        kwargs.pop('week_is_relative', None)
        kwargs.update({
            '{}_is_relative'.format(key): True
            for key in kwargs
        })
        return kwargs

    @classmethod
    def from_wizard_date(cls, wizard_entry):
        start_token = wizard_entry['Tokens']['Begin']
        end_token = wizard_entry['Tokens']['End']
        kwargs = {
            key: value
            for key, value in cls._parse_date_fields(wizard_entry).iteritems()
            if value is not None
        }
        if 'Duration' in wizard_entry:
            kwargs.update(cls._parse_duration(wizard_entry['Duration']))
        return cls(start_token=start_token, end_token=end_token, **kwargs)

    def get_serialized_value(self):
        serialized = {}
        for attribute in ['year', 'month', 'day', 'hour', 'minute']:
            value = getattr(self, attribute, None)
            attribute_is_relative = attribute + '_is_relative'
            value_is_relative = getattr(self, attribute_is_relative, None)
            if value is not None:
                serialized[attribute] = int(value)
                serialized[attribute_is_relative] = value_is_relative
        return serialized


@attr.s(cmp=False)
class DictNamedEntity(NamedEntity):
    """A NamedEnitty that serializes non-None attributes to a dict"""

    def get_serialized_value(self):
        parent_fields = set(attribute.name for attribute in attr.fields(NamedEntity))
        return {
            key: value
            for key, value in attr.asdict(self).iteritems()
            if value is not None and key not in parent_fields
        }


@attr.s(cmp=False)
class FioEntity(DictNamedEntity):

    type = 'YANDEX.FIO'

    first_name = attr.ib(default=None)
    last_name = attr.ib(default=None)
    patronymic_name = attr.ib(default=None)


WIZARD_TO_NER_TYPE = {
    'Country': 'country',
    'City': 'city',
    'Street': 'street',
    'HouseNumber': 'house_number',
    'Airport': 'airport',
}


@attr.s(cmp=False)
class GeoEntity(DictNamedEntity):

    type = 'YANDEX.GEO'

    country = attr.ib(default=None)
    city = attr.ib(default=None)
    street = attr.ib(default=None)
    house_number = attr.ib(default=None)
    airport = attr.ib(default=None)

    @classmethod
    def from_wizard_geoaddr(cls, geoaddr):
        kwargs = {}
        for field in geoaddr.get('Fields', []):
            field_type = field['Type']
            if field_type in WIZARD_TO_NER_TYPE:
                kwargs.setdefault(WIZARD_TO_NER_TYPE[field_type], field['Name'])
        return cls(
            start_token=geoaddr['Tokens']['Begin'],
            end_token=geoaddr['Tokens']['End'],
            **kwargs
        )

    def __nonzero__(self):
        return any([
            self.country,
            self.city,
            self.street,
            self.house_number,
            self.airport,
        ])


def incr_entity_unistat_counter(entity):
    if isinstance(entity, NumberEntity):
        unistat.incr_counter(unistat.CounterName.entities_found_number)
    elif isinstance(entity, DateTimeEntity):
        unistat.incr_counter(unistat.CounterName.entities_found_datetime)
    elif isinstance(entity, FioEntity):
        unistat.incr_counter(unistat.CounterName.entities_found_fio)
    elif isinstance(entity, GeoEntity):
        unistat.incr_counter(unistat.CounterName.entities_found_geo)
    else:
        unistat.incr_counter(unistat.CounterName.entities_found_other)

