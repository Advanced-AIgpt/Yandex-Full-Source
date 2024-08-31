# coding: utf-8

import attr


# todo: make frozen
@attr.s()
class Entity(object):
    start = attr.ib(type=int)
    end = attr.ib(type=int)
    type = attr.ib(type=str)
    value = attr.ib()
    substr = attr.ib(type=str, default='')
    weight = attr.ib(default=None)

    def to_dict(self):
        result = {
            'start': self.start,
            'end': self.end,
            'type': self.type,
            'value': self.value,
        }
        if self.substr:
            result['substr'] = self.substr
        if self.weight is not None:
            result['weight'] = self.weight
        return result
