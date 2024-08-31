# coding: utf-8

import attr

from .base import (
    BaseBlock,
    register,
)


@attr.s
class SuggestBlock(BaseBlock):
    type = attr.ib()
    suggest_type = attr.ib()
    data = attr.ib()
    form_update = attr.ib(default=None)

    @classmethod
    def from_dict(cls, data):
        try:
            res = cls(
                type=data['type'],
                suggest_type=data['suggest_type'],
                data=data.get('data'),
                form_update=data.get('form_update')
            )
            return res
        except KeyError, e:
            raise ValueError('Could not find field %s' % e)


register(SuggestBlock, 'suggest')
