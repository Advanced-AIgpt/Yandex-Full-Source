# coding: utf-8

import attr

from .base import (
    BaseBlock,
    register,
)


@attr.s
class FrameActionBlock(BaseBlock):
    type = attr.ib()
    action_id = attr.ib()
    frame_action = attr.ib()

    @classmethod
    def from_dict(cls, data):
        return cls(
            type=data['type'],
            action_id=data.get('data', {}).get('action_id'),
            frame_action=data.get('data', {}).get('frame_action'),
        )


register(FrameActionBlock, 'frame_action')
