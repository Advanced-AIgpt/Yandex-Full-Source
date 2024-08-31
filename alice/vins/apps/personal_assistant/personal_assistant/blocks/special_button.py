# coding: utf-8

import attr

from .base import BaseBlock, register


@attr.s
class SpecialButtonBlock(BaseBlock):
    type = attr.ib()
    button_type = attr.ib()
    data = attr.ib()

    @classmethod
    def from_dict(cls, data):
        try:
            res = cls(
                type=data['type'],
                button_type=data['button_type'],
                data=data.get('data')
            )
            return res
        except KeyError, e:
            raise ValueError('Could not find field %s' % e)


register(SpecialButtonBlock, 'special_button')
