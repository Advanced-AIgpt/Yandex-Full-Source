# coding: utf-8
import attr
from .base import BaseBlock, register


@attr.s
class DebugInfoBlock(BaseBlock):
    type = attr.ib()
    data = attr.ib()

    @classmethod
    def from_dict(cls, data):
        return cls(
            type=data['type'],
            data=data.get('data'),
        )


register(DebugInfoBlock, 'debug_info')
