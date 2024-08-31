#!/usr/bin/env python
# -*- coding: utf-8 -*-


class LoginsInfo(object):

    def __init__(self):
        self.puids = []
        self.pluses = []
        self.gifts = []

    def update_field(self, field, values, record):
        value = record[field]
        if len(values) == 0 or value != values[-1][1]:
            values.append((record['event_timestamp'], value))
        return values

    def add_event(self, record):
        self.puids = self.update_field('puid', self.puids, record)
        self.pluses = self.update_field('has_plus', self.pluses, record)
        self.gifts = self.update_field('active_tv_gift', self.gifts, record)

    def to_dict(self, obj):
        return {str(k): v for k, v in obj}

    def get_puids(self):
        return self.to_dict(self.puids)

    def get_pluses(self):
        return self.to_dict(self.pluses)

    def get_gifts(self):
        return self.to_dict(self.gifts)
