# coding: utf-8

from __future__ import unicode_literals

import attr

from .base import BaseAnnotation, register_annotation


@attr.s
class WizardAnnotation(BaseAnnotation):
    markup = attr.ib()
    rules = attr.ib()
    token_alignment = attr.ib(default=None)

    @classmethod
    def from_dict(cls, data):
        return cls(
            markup=data.get('markup'),
            rules=data.get('rules'),
            token_alignment=data.get('token_alignment')
        )

    def to_dict(self):
        return {'markup': self.markup, 'rules': self.rules, 'token_alignment': self.token_alignment}

    def trim(self):
        """Drop `markup` entries which are needed only in WizardFeaturesExtractor.

        Retain only 'Tokens' and 'Morph' which are needed for anaphora resolution.
        """

        self.markup = {
            'Tokens': self.markup.get('Tokens', []),
            'Morph': self.markup.get('Morph', [])
        }
        self.token_alignment = None


register_annotation(WizardAnnotation, 'wizard')
