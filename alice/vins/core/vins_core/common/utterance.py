# -*- coding: utf-8 -*-

from __future__ import unicode_literals

import json

import attr

from vins_core.schema import features_pb2
from vins_core.schema.interface import PbSerializable
from vins_core.utils.strings import fix_protobuf_string


@attr.s(frozen=True, cmp=True)
class Utterance(PbSerializable):
    """General class for holding input utterance and its source ('voice' or 'text')

    Any `BaseConnector` child class uses `Utterance` in `handle_utterance()` method.
    Any `VinsApp` child class uses `Utterance` in `handle_utterance()` method.
    `FormFillingDialogManager` uses `Utterance` in `handle()` method.
    """
    _PB_CLS = features_pb2.Utterance

    VOICE_INPUT_SOURCE = 'voice'
    TEXT_INPUT_SOURCE = 'text'
    SUGGESTED_INPUT_SOURCE = 'suggested'
    IMAGE_INPUT_SOURCE = 'image'
    MUSIC_INPUT_SOURCE = 'music'

    text = attr.ib()
    input_source = attr.ib(default=TEXT_INPUT_SOURCE)
    payload = attr.ib(default=None)
    hypothesis_number = attr.ib(default=None)
    end_of_utterance = attr.ib(default=True)

    @classmethod
    def from_string(cls, text):
        return cls(text)

    def to_dict(self):
        d = {'text': self.text, 'input_source': self.input_source}
        if self.payload:
            d['payload'] = self.payload
        return d

    @classmethod
    def from_dict(cls, obj):
        return cls(obj['text'], obj['input_source'], obj.get('payload'))

    def to_pb(self):
        if self.payload:
            return features_pb2.Utterance(
                text=self.text,
                input_source=self.input_source,
                payload=json.dumps(self.payload)
            )
        else:
            return features_pb2.Utterance(text=self.text, input_source=self.input_source)

    @classmethod
    def from_pb(cls, pb_obj):
        if pb_obj.payload:
            return cls(text=fix_protobuf_string(pb_obj.text), input_source=pb_obj.input_source, payload=json.loads(pb_obj.payload))
        else:
            return cls(text=fix_protobuf_string(pb_obj.text), input_source=pb_obj.input_source)

    def __unicode__(self):
        return self.text
