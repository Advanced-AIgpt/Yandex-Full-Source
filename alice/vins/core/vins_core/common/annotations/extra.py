# coding utf-8

from __future__ import unicode_literals
import attr
from .base import BaseAnnotation, register_annotation


@attr.s
class ExtraAnnotation(BaseAnnotation):
    rooms = attr.ib(default=attr.Factory(list))
    groups = attr.ib(default=attr.Factory(list))
    devices = attr.ib(default=attr.Factory(list))
    multiroom_all_devices = attr.ib(default=attr.Factory(list))

    @classmethod
    def from_dict(cls, data):
        return cls(
            rooms=data.get('rooms'),
            groups=data.get('groups'),
            devices=data.get('devices'),
            multiroom_all_devices=data.get('multiroom_all_devices')
        )

    def to_dict(self):
        return {
            'rooms': self.rooms,
            'groups': self.groups,
            'devices': self.devices,
            'multiroom_all_devices': self.multiroom_all_devices,
        }


register_annotation(ExtraAnnotation, 'extra')
