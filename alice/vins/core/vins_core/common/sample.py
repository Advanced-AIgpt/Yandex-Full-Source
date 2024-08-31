# -*- coding: utf-8 -*-

from __future__ import unicode_literals

import json

from alice.nlu.py_libs.utils import sample as simple_sample_m

from vins_core.common.annotations import AnnotationsBag
from vins_core.common.utterance import Utterance
from vins_core.schema import features_pb2
from vins_core.schema.interface import PbSerializable
from vins_core.utils.strings import smart_unicode, smart_utf8, fix_protobuf_string
from vins_core.common.annotations.extra import ExtraAnnotation


class Sample(PbSerializable):
    """Class which represents Sample. Samples are used everywhere in `vins_core.[dm,nlu,ner]`.

    Args:
        tokens (list of basestring): List of tokens.
        utterance (vins_core.dm.utterance.Utterance): String representing utterance.
        tags (list of basestring): List of tags.
        annotations {AnnotationsBag, dict of BaseAnnotation}: List of sample annotations.
    """
    _PB_CLS = features_pb2.Sample

    def __init__(self, tokens=None, utterance=None, tags=None, annotations=None, weight=1.0,
                 app_id='', partially_normalized_text=None):
        self._utterance = utterance or Utterance('')
        self._tokens = tokens or []
        self._weight = weight
        self._partially_normalized_text = partially_normalized_text
        if tags:
            self._tags = tags
            self._has_tags = True
        else:
            self._tags = ['O'] * len(self.tokens)
            self._has_tags = False
        self.app_id = app_id

        if not isinstance(annotations, AnnotationsBag):
            self._annotations = AnnotationsBag(bag=annotations)
        else:
            self._annotations = annotations.copy()

        self._check_params()

    def assign(self, other):
        self._utterance = other._utterance
        self._tokens = other._tokens
        self._weight = other._weight
        self._tags = other._tags
        self._has_tags = other._has_tags
        self.app_id = other.app_id
        self._annotations = other._annotations

    def _check_params(self):
        if self.utterance is not None and not isinstance(self.utterance, Utterance):
            raise ValueError("utterance must be instance of Utterance class."
                             "You passed type {}".format(self.utterance.type))

        if len(self.tokens) != len(self.tags):
            raise ValueError("Lengths of tokens and tags must be equal."
                             "You passed tokens={}, tags={}".format(self.tokens, self.tags))

    @property
    def annotations(self):
        return self._annotations

    @property
    def utterance(self):
        return self._utterance

    @property
    def text(self):
        return ' '.join(self.tokens).strip()

    @property
    def weight(self):
        return self._weight

    @property
    def tokens(self):
        return self._tokens

    @property
    def tags(self):
        return self._tags

    @property
    def has_tags(self):
        return self._has_tags

    @property
    def partially_normalized_text(self):
        if self._partially_normalized_text is None:
            return self.text
        return self._partially_normalized_text

    def __bool__(self):
        return bool(self.tokens)

    def __nonzero__(self):
        return self.__bool__()

    def __hash__(self):
        return hash((
            self.text,
            tuple(self.tokens),
            tuple(self.tags)
        ))

    def __eq__(self, other):
        return all((
            self.utterance == other.utterance,
            tuple(self.tokens) == tuple(other.tokens),
            tuple(self.tags) == tuple(other.tags)
        ))

    def __ne__(self, other):
        return not self.__eq__(other)

    @classmethod
    def from_nlu_source_item(cls, item):
        simple_sample = simple_sample_m.Sample.from_nlu_source_item(item)
        annotations = {'extra': cls._parse_extra_as_annotation(nlu_source_item=item)} if item.extra else None
        return cls.from_string(simple_sample.text, tokens=simple_sample.tokens, tags=simple_sample.tags, annotations=annotations)

    @staticmethod
    def _parse_extra_as_annotation(nlu_source_item):
        if nlu_source_item.extra:
            try:
                extra_json = json.loads(nlu_source_item.extra)
                rooms, groups, devices, multiroom_all_devices = (
                    [smart_unicode(tokens) for tokens in extra_json.get(location_type, [])]
                    for location_type in ['rooms', 'groups', 'devices', 'multiroom_all_devices']
                )
                return ExtraAnnotation(rooms=rooms, groups=groups, devices=devices,
                                       multiroom_all_devices=multiroom_all_devices)
            except Exception as e:
                err_msg = 'Something wrong with extra in sample {} with extra {}; exception is {}'.format(
                    nlu_source_item.text, nlu_source_item.extra, e
                )
                raise RuntimeError(err_msg)

    @classmethod
    def from_string(cls, item, tokens=None, tags=None, annotations=None):
        utterance = Utterance(item)
        return cls.from_utterance(utterance, tokens=tokens, tags=tags, annotations=annotations)

    @classmethod
    def from_weighted_string(cls, item, tokens=None, tags=None, annotations=None):
        utterance = Utterance(item.string)
        return cls.from_utterance(utterance, tokens=tokens, tags=tags, annotations=annotations, weight=item.weight)

    @classmethod
    def from_utterance(cls, utterance, tokens=None, tags=None, annotations=None, weight=1.0):
        text = smart_unicode(utterance.text)
        tokens = tokens or text.split()
        tags = tags or ['O'] * len(tokens)

        return cls(tokens=tokens, utterance=utterance, tags=tags, annotations=annotations, weight=weight)

    @classmethod
    def from_none(cls):
        return cls()

    def __repr__(self):
        return smart_utf8('Sample(text={0}, tokens=[{1}], tags=[{2}], annotations={3})'.format(
            self.text,
            ' '.join(self.tokens),
            ' '.join(self.tags),
            self.annotations.keys()
        ))

    def __len__(self):
        return len(self._tokens)

    def to_pb(self):
        utt_pb = self.utterance.to_pb()
        annotations_str = json.dumps(self.annotations.to_dict())
        return features_pb2.Sample(
            tokens=self.tokens,
            tags=self.tags,
            weight=self.weight,
            utterance=utt_pb,
            annotations_bag=annotations_str,
        )

    def to_dict(self):
        return dict(
            tokens=self.tokens,
            tags=self.tags,
            weight=self.weight,
            utterance=self.utterance.to_dict(),
            annotations_bag=self.annotations.to_dict(),
        )

    @classmethod
    def from_pb(cls, pb_obj):
        utterance = Utterance.from_pb(pb_obj.utterance)
        annotations = AnnotationsBag.from_dict(json.loads(pb_obj.annotations_bag))
        return cls(
            tokens=map(fix_protobuf_string, pb_obj.tokens), tags=map(fix_protobuf_string, pb_obj.tags),
            weight=pb_obj.weight, utterance=utterance, annotations=annotations
        )

    @classmethod
    def from_dict(cls, data):
        utterance = Utterance.from_dict(data.get('utterance'))
        annotations = AnnotationsBag.from_dict(data.get('annotations'))
        return cls(
            tokens=data.get('tokens'),
            tags=data.get('tags'),
            weight=data.get('weight'),
            utterance=utterance,
            annotations=annotations,
        )
