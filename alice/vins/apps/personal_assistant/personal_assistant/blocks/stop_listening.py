# coding: utf-8

import attr

from .base import BaseBlock, register


@attr.s
class StopListeningBlock(BaseBlock):
    type = attr.ib()
    data = attr.ib()

    @classmethod
    def from_dict(cls, data):
        try:
            res = cls(
                type=data['type'],
                data=data.get('data')
            )
            return res
        except KeyError, e:
            raise ValueError('Could not find field %s' % e)


register(StopListeningBlock, 'stop_listening')
