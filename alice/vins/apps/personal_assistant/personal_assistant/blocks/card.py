# coding: utf-8

import attr

from .base import (
    BaseBlock,
    register,
)


@attr.s
class CardBlock(BaseBlock):
    type = attr.ib()
    card_template = attr.ib()
    card_layout = attr.ib()
    data = attr.ib()

    @classmethod
    def from_dict(cls, data):
        try:
            res = cls(
                type=data['type'],
                card_template=data.get('card_template'),
                card_layout=data.get('card_layout'),
                data=data.get('data')
            )
            return res
        except KeyError, e:
            raise ValueError('Could not find field %s' % e)


register(CardBlock, 'div_card')


@attr.s
class Div2CardBlock(BaseBlock):
    type = attr.ib()
    body = attr.ib()
    hide_borders = attr.ib()
    data = attr.ib()
    card_template = attr.ib()
    templates = attr.ib()
    template_names = attr.ib()
    text = attr.ib()
    text_template = attr.ib()

    @classmethod
    def from_dict(cls, data):
        try:
            res = cls(
                type=data['type'],
                body=data.get('body'),
                hide_borders=data.get('hide_borders', False),
                data=data.get('data'),
                card_template=data.get('card_template'),
                template_names=data.get('template_names', []),
                templates=data.get('templates', {}),
                text=data.get('text'),
                text_template=data.get('text_template'),
            )
            return res
        except KeyError, e:
            raise ValueError('Could not find field %s' % e)


register(Div2CardBlock, 'div2_card')


@attr.s
class TextCardBlock(BaseBlock):
    type = attr.ib()
    phrase_id = attr.ib()
    data = attr.ib()

    @classmethod
    def from_dict(cls, data):
        try:
            res = cls(
                type=data['type'],
                phrase_id=data['phrase_id'],
                data=data.get('data')
            )
            return res
        except KeyError, e:
            raise ValueError('Could not find field %s' % e)


register(TextCardBlock, 'text_card')
