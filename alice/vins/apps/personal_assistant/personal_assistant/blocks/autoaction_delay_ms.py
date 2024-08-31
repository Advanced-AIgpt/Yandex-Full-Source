# coding: utf-8

import attr

from .base import BaseBlock, register


@attr.s
class AutoactionDelayMsBlock(BaseBlock):
    type = attr.ib()
    delay_ms = attr.ib()

    @classmethod
    def from_dict(cls, data):
        try:
            res = cls(
                type=data['type'],
                delay_ms=data.get('delay_ms')
            )
            return res
        except KeyError, e:
            raise ValueError('Could not find field %s' % e)


register(AutoactionDelayMsBlock, 'autoaction_delay_ms')
