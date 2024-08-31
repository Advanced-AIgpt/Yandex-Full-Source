# coding: utf-8

from __future__ import unicode_literals

import attr

from .base import BaseAnnotation, register_annotation


@attr.s
class Entity(object):
    name = attr.ib()
    type = attr.ib()
    id = attr.ib(default='')
    tags = attr.ib(default=[])
    start = attr.ib(default=None)  # Left entity boundary in the original sample.
    end = attr.ib(default=None)    # Right entity boundary in the original sample (exclusive).

    @classmethod
    def from_dict(cls, obj):
        return cls(
            name=obj.get('name'),
            type=obj.get('type'),
            id=obj.get('id'),
            tags=obj.get('tags'),
            start=obj.get('start'),
            end=obj.get('end')
        )

    def to_dict(self):
        return attr.asdict(self)


@attr.s(frozen=True)
class EntityFeatures(object):
    tags = attr.ib(default=(), converter=tuple)
    site_ids = attr.ib(default=(), converter=tuple)
    has_music_info = attr.ib(default=False, converter=bool)
    types = attr.ib(default=(), converter=tuple)
    subtypes = attr.ib(default=(), converter=tuple)

    @classmethod
    def from_dict(cls, obj):
        return cls(
            tags=obj.get('tags', ()),
            site_ids=obj.get('site_ids', ()),
            has_music_info=obj.get('has_music_info', False),
            types=obj.get('types', ()),
            subtypes=obj.get('subtypes', ())
        )

    def to_dict(self):
        return attr.asdict(self)


@attr.s
class EntitySearchAnnotation(BaseAnnotation):
    entities = attr.ib(default=[])
    entity_features = attr.ib(default=EntityFeatures())

    @classmethod
    def from_dict(cls, data):
        entity_features_dict = data.get('entity_features', {}) or {}
        return cls(entities=[Entity.from_dict(ent) for ent in data.get('entities', [])],
                   entity_features=EntityFeatures.from_dict(entity_features_dict))

    def trim(self):
        """Drops `entity_features` needed only in BagOfEntitySearchFeaturesFactor.
        """
        self.entity_features = None


register_annotation(EntitySearchAnnotation, 'entitysearch')
