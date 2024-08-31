# coding: utf-8
from __future__ import unicode_literals

import logging

from vins_core.common.utterance import Utterance

logger = logging.getLogger(__name__)

_events_map = {}


def make_asr_result(text):
    return [
        {
            'utterance': text,
            'confidence': 1.0,
            'words': [
                {
                    'value': word,
                    'confidence': 1.0
                } for word in text.split()
            ]
        }
    ]


def register_event(name, e):
    _events_map[name] = e


class RequestEvent(object):
    def __init__(self, payload=None, **kwargs):
        self._payload = payload or {}

    @property
    def payload(self):
        return self._payload

    @staticmethod
    def from_dict(dict_obj):
        event_type = dict_obj.get('type')
        event_class = _events_map.get(event_type)
        if not event_class:
            raise ValueError('Unknown event type "%s"' % event_type)

        class_params = dict_obj.copy()
        if 'type' in class_params:
            del class_params['type']

        return event_class(**class_params)


class TextInputEvent(RequestEvent):
    event_type = 'text_input'
    utterance_type = Utterance.TEXT_INPUT_SOURCE

    def __init__(self, text, **kwargs):
        super(TextInputEvent, self).__init__(**kwargs)
        if text:
            assert isinstance(text, basestring), "TextInputEvent must be instantiated with string, not %s" % type(text)
        self._text = text

    @property
    def utterance(self):
        return Utterance(self._text, input_source=self.utterance_type)

    def to_dict(self):
        return {
            'type': self.event_type,
            'text': self._text,
            'payload': self._payload,
        }


class SuggestedInputEvent(TextInputEvent):
    event_type = 'suggested_input'
    utterance_type = Utterance.SUGGESTED_INPUT_SOURCE


class VoiceInputEvent(RequestEvent):
    event_type = 'voice_input'

    def __init__(self, asr_result, biometry_scoring=None, hypothesis_number=None, end_of_utterance=True,
                 biometry_classification=None, **kwargs):
        super(VoiceInputEvent, self).__init__(**kwargs)
        self._asr_result = asr_result
        self._hypothesis_number = hypothesis_number
        self._end_of_utterance = end_of_utterance
        self._biometry_scoring = biometry_scoring
        self._biometry_classification = biometry_classification

    def context_hint(self, name):
        best_result = self._asr_result[0]

        if best_result.get('context_hints'):
            return best_result['context_hints'].get(name)

        return None

    @property
    def utterance(self):
        best_result = self._asr_result[0]
        # Use pre-normalized text here as we're going to run the normalizer ourselves
        if best_result.get('words'):
            utterance = ' '.join(word['value'] for word in best_result['words'])
        else:
            utterance = best_result['utterance']

        return Utterance(
            utterance,
            input_source=Utterance.VOICE_INPUT_SOURCE,
            hypothesis_number=self._hypothesis_number,
            end_of_utterance=self._end_of_utterance,
        )

    def biometrics_scores(self):
        return self._biometry_scoring

    def biometry_classification(self):
        return self._biometry_classification

    def asr_utterance(self):
        return self._asr_result[0]["utterance"]

    def to_dict(self):
        dict_obj = {
            'type': self.event_type,
            'asr_result': self._asr_result,
            'payload': self._payload,
            'end_of_utterance': self._end_of_utterance,
        }

        if self._hypothesis_number is not None:
            dict_obj['hypothesis_number'] = self._hypothesis_number

        if self._biometry_scoring:
            dict_obj['biometry_scoring'] = self._biometry_scoring

        if self._biometry_classification:
            dict_obj['biometry_classification'] = self._biometry_classification

        return dict_obj

    @classmethod
    def from_utterance(cls, utterance, **kwargs):
        return cls(asr_result=[
            {
                'utterance': utterance,
                'confidence': 1.0,
                'words': [
                    {
                        'value': word,
                        'confidence': 1.0
                    } for word in utterance.split()
                ]
            }
        ], **kwargs)


class ServerActionEvent(RequestEvent):
    event_type = 'server_action'

    def __init__(self, name, **kwargs):
        super(ServerActionEvent, self).__init__(**kwargs)
        self._name = name

    @property
    def utterance(self):
        return None

    @property
    def action_name(self):
        return self._name

    def to_dict(self):
        return {
            'type': self.event_type,
            'name': self._name,
            'payload': self._payload,
        }


class MusicInputEvent(RequestEvent):
    event_type = 'music_input'

    def __init__(self, music_result, **kwargs):
        super(MusicInputEvent, self).__init__(**kwargs)
        self._music_result = music_result

    @property
    def utterance(self):
        return Utterance(
            text=None,
            input_source=Utterance.MUSIC_INPUT_SOURCE,
            payload={
                'data': self._music_result.get('data'),
                'result': self._music_result.get('result'),
                'error_text': self._music_result.get('error_text')
            }
        )

    def to_dict(self):
        return {
            'type': self.event_type,
            'music_result': self._music_result,
            'payload': self._payload,
        }


class ImageInputEvent(RequestEvent):
    event_type = 'image_input'

    def __init__(self, **kwargs):
        super(ImageInputEvent, self).__init__(**kwargs)

    @property
    def utterance(self):
        return Utterance(
            text=None,
            input_source=Utterance.IMAGE_INPUT_SOURCE,
            payload={
                'data': self._payload,
            }
        )

    def to_dict(self):
        return {
            'type': self.event_type,
            'payload': self._payload,
        }


EVENTS = (
    TextInputEvent,
    VoiceInputEvent,
    ServerActionEvent,
    SuggestedInputEvent,
    MusicInputEvent,
    ImageInputEvent,
)


for e in EVENTS:
    register_event(e.event_type, e)
