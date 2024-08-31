# coding: utf-8
from __future__ import unicode_literals
import attr
from .base import BaseAnnotation, register_annotation


@attr.s
class FlagAnnotation(BaseAnnotation):
    value = attr.ib(default=False)
    value_type = attr.ib(default='bool')

    @classmethod
    def from_dict(cls, data):
        return cls(data.get('value', False), data.get('value_type', 'bool'))


register_annotation(FlagAnnotation, 'flag')
