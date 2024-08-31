# coding: utf-8

import attr
import json

from .base import (
    BaseBlock,
    register,
)


@attr.s
class CommandBlock(BaseBlock):
    type = attr.ib()
    command_type = attr.ib()
    command_sub_type = attr.ib()
    data = attr.ib()

    @classmethod
    def from_dict(cls, data):
        try:
            res = cls(
                type=data['type'],
                command_type=data['command_type'],
                command_sub_type=data.get('command_sub_type'),
                data=data.get('data')
            )
            return res
        except KeyError, e:
            raise ValueError('Could not find field %s' % e)


register(CommandBlock, 'command')


@attr.s
class UniproxyActionBlock(BaseBlock):
    type = attr.ib()
    command_type = attr.ib()
    data = attr.ib()

    @classmethod
    def from_dict(cls, data):
        try:
            res = cls(
                type=data['type'],
                command_type=data['command_type'],
                data=data.get('data')
            )
            return res
        except KeyError, e:
            raise ValueError('Could not find field %s' % e)


register(UniproxyActionBlock, 'uniproxy-action')


@attr.s
class TypedSemanticFrameBlock(BaseBlock):
    type = attr.ib()
    payload = attr.ib()
    analytics = attr.ib()

    @classmethod
    def from_dict(cls, data):
        try:
            res = cls(
                type=data['type'],
                payload=json.loads(data.get('payload', '{}')),
                analytics=json.loads(data.get('analytics', '{}'))
            )
            return res
        except KeyError, e:
            raise ValueError('Could not find field %s' % e)


register(TypedSemanticFrameBlock, 'typed_semantic_frame')
