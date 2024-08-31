# coding: utf-8

import attr

from personal_assistant.blocks.base import (
    BaseBlock,
    register,
)


@attr.s
class SensitiveBlock(BaseBlock):
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


register(SensitiveBlock, 'sensitive')
