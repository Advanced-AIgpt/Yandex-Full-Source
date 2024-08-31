# coding: utf-8
from __future__ import unicode_literals
import attr
from .base import BaseAnnotation, register_annotation


@attr.s
class NerAnnotation(BaseAnnotation):
    # This annotation is used only for classification and is not designed for saving and restoring
    entities = attr.ib(default=[])

    @classmethod
    def from_dict(cls, data):
        return cls()

    def trim(self):
        self.entities = []


register_annotation(NerAnnotation, 'ner')
