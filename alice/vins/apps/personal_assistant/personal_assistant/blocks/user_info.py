# coding: utf-8

import attr

from .base import BaseBlock, register


@attr.s
class UserInfoBlock(BaseBlock):
    type = attr.ib()
    username = attr.ib()
    is_silent = attr.ib()

    @classmethod
    def from_dict(cls, data):
        try:
            res = cls(
                type=data['type'],
                username=data.get('user_name'),
                is_silent=data.get('is_silent', False)
            )
            return res
        except KeyError, e:
            raise ValueError('Could not find field %s' % e)


register(UserInfoBlock, 'user_info')
