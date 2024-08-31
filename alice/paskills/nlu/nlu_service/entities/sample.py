# coding: utf-8

import attr

from nlu_service.utils.string import ensure_unicode, smart_utf8

# copy-pasted from vins


@attr.s(frozen=True, cmp=True)
class Utterance(object):
    """General class for holding input utterance and its source ('voice' or 'text')

    Any `BaseConnector` child class uses `Utterance` in `handle_utterance()` method.
    Any `VinsApp` child class uses `Utterance` in `handle_utterance()` method.
    `FormFillingDialogManager` uses `Utterance` in `handle()` method.
    """
    text = attr.ib()

    @classmethod
    def from_string(cls, text):
        return cls(text)

    def __unicode__(self):
        return self.text


class Sample(object):
    """Class which represents Sample. Samples are used everywhere in `vins_core.[dm,nlu,ner]`.

    Args:
        tokens (list of basestring): List of tokens.
        utterance (vins_core.dm.utterance.Utterance): String representing utterance.
        tags (list of basestring): List of tags.
        annotations {AnnotationsBag, dict of BaseAnnotation}: List of sample annotations.
    """

    def __init__(self, tokens=None, utterance=None, tags=None, annotations=None, weight=1.0, app_id=''):
        self._utterance = utterance or Utterance('')
        self._tokens = tokens or []
        self._weight = weight
        if tags:
            self._tags = tags
            self._has_tags = True
        else:
            self._tags = ['O'] * len(self.tokens)
            self._has_tags = False
        self.app_id = app_id

        self._check_params()

    def _check_params(self):
        if self.utterance is not None and not isinstance(self.utterance, Utterance):
            raise ValueError("utterance must be instance of Utterance class."
                             "You passed type {}".format(self.utterance.type))

        if len(self.tokens) != len(self.tags):
            raise ValueError("Lengths of tokens and tags must be equal."
                             "You passed tokens={}, tags={}".format(self.tokens, self.tags))

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
    def from_string(cls, item, tokens=None, tags=None, annotations=None):
        utterance = Utterance(item)
        return cls.from_utterance(utterance, tokens=tokens, tags=tags, annotations=annotations)

    @classmethod
    def from_utterance(cls, utterance, tokens=None, tags=None, annotations=None, weight=1.0):
        text = ensure_unicode(utterance.text)
        tokens = tokens or text.split()
        tags = tags or ['O'] * len(tokens)

        return cls(tokens=tokens, utterance=utterance, tags=tags, annotations=annotations, weight=weight)

    @classmethod
    def from_none(cls):
        return cls()

    def __repr__(self):
        return smart_utf8('Sample(text={0}, tokens=[{1}], tags=[{2}])'.format(
            self.text,
            ' '.join(self.tokens),
            ' '.join(self.tags),
        ))

    def __len__(self):
        return len(self._tokens)
