# coding: utf-8

from __future__ import unicode_literals

import re
import uuid
from datetime import datetime

import attr

from vins_core.common.utterance import Utterance
from vins_core.common.annotations import AnnotationsBag
from vins_core.utils.data import uuid_to_str
from vins_core.utils.datetime import utcnow, datetime_to_timestamp, timestamp_to_datetime
from .form_filler.models import Form
from .response import VinsResponse

instance_of = attr.validators.instance_of


def maybe_instance_of(cls):
    return attr.validators.optional(instance_of(cls))


@attr.s
class DialogTurn(object):
    dt = attr.ib(validator=instance_of(datetime))
    utterance = attr.ib(validator=maybe_instance_of(Utterance))
    response = attr.ib(validator=maybe_instance_of(VinsResponse))
    form = attr.ib(validator=maybe_instance_of(Form))
    response_text = attr.ib(validator=maybe_instance_of(basestring))
    voice_text = attr.ib(validator=maybe_instance_of(basestring))
    annotations = attr.ib(validator=maybe_instance_of(AnnotationsBag))

    def to_dict(self):
        return {
            'dt': datetime_to_timestamp(self.dt),
            'utterance': self.utterance and self.utterance.to_dict(),
            'response': self.response and self.response.to_dict(),
            'form': self.form and self.form.to_dict(),
            'response_text': self.response_text,
            'voice_text': self.voice_text,
            'annotations': self.annotations and self.annotations.to_dict()
        }

    @classmethod
    def from_dict(cls, obj):
        if obj.get('annotations') is not None:
            annotations = AnnotationsBag.from_dict(obj['annotations'], strict=False)
        else:
            annotations = None

        return cls(
            dt=timestamp_to_datetime(obj['dt']),
            utterance=obj['utterance'] and Utterance.from_dict(obj['utterance']),
            response=obj['response'] and VinsResponse.from_dict(obj['response']),
            form=obj['form'] and Form.from_dict(obj['form']),
            response_text=obj['response_text'],
            voice_text=obj['voice_text'],
            annotations=annotations
        )


@attr.s
class Phrase(object):
    text = attr.ib(validator=instance_of(basestring))
    sender = attr.ib(validator=attr.validators.in_(['vins', 'user']))
    annotations = attr.ib(validator=maybe_instance_of(AnnotationsBag), default=None)


def remove_tts_markup(text):
    return re.sub(r'(\B(#\w+|<\[?.*?\]?>|\.sil<\[.*?\]>))|\b\+\b', '', text, flags=re.UNICODE)


def remove_spaces(text):
    if text is None:
        return None
    else:
        return re.sub(r'\s+', ' ', text, flags=re.U)


class DialogHistory(object):
    MAX_TURNS = 3

    def __init__(self, max_turns=MAX_TURNS):
        self._collection = []
        self._max_turns = max_turns

    def __len__(self):
        return len(self._collection)

    def __iter__(self):
        return iter(self._collection)

    def add(self, utterance, response, form=None, datetime=None, annotations=None):
        datetime = datetime or utcnow()

        turn = DialogTurn(
            dt=datetime,
            utterance=utterance,
            response=response,
            form=form,
            response_text=self._get_response_text(response),
            voice_text=response.voice_text,
            annotations=annotations
        )

        # remove responses from all except newest turn to save space in db
        for record in self._collection:
            record.response = None

        self._collection.append(turn)
        self._collection = self._collection[-self._max_turns:]

    def _get_response_text(self, response):
        vins_text = None
        if response and len(response.cards) > 0:
            card = response.cards[0]
            if card.type in ('text_with_button', 'simple_text'):
                vins_text = card.text

        if vins_text is None and response.voice_text:
            vins_text = remove_tts_markup(response.voice_text)

        return remove_spaces(vins_text)

    def last(self, count=None):
        if count is None:
            if self._collection:
                for item in reversed(self._collection):
                    if item.voice_text:
                        return item
                return self._collection[-1]
            else:
                return None
        else:
            return list(reversed(self._collection[-count:]))

    def _last_phrases(self, count):
        for turn in self._collection[-count:]:
            if turn.utterance and turn.utterance.text is not None:
                yield Phrase(text=turn.utterance.text, sender='user', annotations=turn.annotations)

            if turn.response_text:
                yield Phrase(text=turn.response_text, sender='vins', annotations=turn.annotations)

    def last_phrases(self, count):
        return list(self._last_phrases(count))[-count:]

    def clear(self):
        self._collection = []

    def to_dict(self):
        return {
            # unused app_id and uuid for backward compatibility,
            # TODO remove after migration
            'app_id': 'vins',
            'uuid': uuid_to_str(uuid.UUID('deadbeef-6898-4c0a-af93-7a8821aab244')),

            'turns': [turn.to_dict() for turn in self._collection],
        }

    @classmethod
    def from_dict(cls, obj):
        dh = cls()
        collection = []

        for turn in obj['turns']:
            collection.append(
                DialogTurn.from_dict(turn)
            )

        dh._collection = collection
        return dh
