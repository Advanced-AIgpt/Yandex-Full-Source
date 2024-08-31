# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import logging

import attr
import ya_light_syntax

from vins_core.common.sample import Sample
from vins_core.utils.strings import smart_utf8
from vins_core.utils.iter import all_chains

logger = logging.getLogger(__name__)

_REGISTERED_PRONOUNS = frozenset(['он', 'она', 'оно', 'они',
                                  'тот', 'туда', 'оттуда',
                                  'там', 'здесь', 'сюда', 'отсюда'])


class SyntaxParsingException(Exception):
    pass


class SyntaxUnitInterface(object):
    def __init__(self, sentence):
        self._sentence = sentence

    @property
    def grammem(self):
        """All morphological properties combined in one string.
        """
        raise NotImplementedError

    @property
    def start(self):
        """Start char position in the sentence.
        """
        raise NotImplementedError

    @property
    def end(self):
        """End char position in the sentence.
        """
        raise NotImplementedError

    @property
    def pos(self):
        """Part-of-speech tag of single word if child class represents just one token,
        or unit head word if there are several tokens.
        """
        raise NotImplementedError

    @property
    def sg_pl(self):
        """Singular/plural value of single word if child class represents just one token,
        or unit head word if there are several tokens.
        """
        raise NotImplementedError

    @property
    def gender(self):
        """Gender value of single word if child class represents just one token,
        or unit head word if there are several tokens.
        """
        raise NotImplementedError

    @property
    def anim(self):
        """Animacy value. Can be "anim" or "inan".
        """
        raise NotImplementedError

    @property
    def text(self):
        """Textual value of unit. E.g. "аптека", "он", ...
        """
        raise NotImplementedError

    @property
    def geo(self):
        """Boolean flag which means unit is geographical name"""
        raise NotImplementedError

    @property
    def head(self):
        """Return head of unit. For tokens, head is token itself."""
        raise NotImplementedError

    @property
    def sentence(self):
        """Textual value of unit's sentence.
        """
        return self._sentence

    @property
    def type(self):
        """String value representing unit type. E.g., 'pronoun', 'noun', 'pp', 'np'
        """
        raise NotImplementedError


class Token(SyntaxUnitInterface):
    PRONOUN_TYPE = 'pronoun'
    NOUN_TYPE = 'noun'
    OTHER_TYPE = 'other'

    def __init__(self, sentence, text, lemma, grammem, start, end, other_grammems=None):
        super(Token, self).__init__(sentence)
        self._start = start
        self._end = end
        self._text = text
        self._lemma = lemma
        self._grammem = grammem.split()
        self._other_grammems = [gr.split() for gr in other_grammems] if other_grammems else None

    def _find_in_grammem(self, list_of_values):
        for v in list_of_values:
            if v in self._grammem:
                return v
        return None

    @property
    def grammem(self):
        return ' '.join(self._grammem)

    @property
    def start(self):
        return self._start

    @property
    def end(self):
        return self._end

    @property
    def pos(self):
        values = ['ADV', 'ADVPRO', 'ANUM', 'APRO', 'ART', 'COM', 'CONJ',
                  'INTJ', 'NUM', 'PART', 'PR', 'S', 'SPRO', 'V']
        return self._find_in_grammem(values)

    @property
    def geo(self):
        values = ['geo']
        return True if self._find_in_grammem(values) else False

    @property
    def sg_pl(self):
        values = ['sg', 'pl']
        return self._find_in_grammem(values)

    @property
    def gender(self):
        values = ['m', 'f', 'n']
        return self._find_in_grammem(values)

    @property
    def anim(self):
        values = ['anim', 'inan']
        return self._find_in_grammem(values)

    @property
    def text(self):
        return self._text

    @property
    def lemma(self):
        return self._lemma

    @property
    def head(self):
        return self

    @property
    def type(self):
        if self.pos in ['SPRO', 'APRO', 'ADVPRO'] and self.lemma in _REGISTERED_PRONOUNS:
            return self.PRONOUN_TYPE
        elif self.pos == 'S':
            return self.NOUN_TYPE
        else:
            return self.OTHER_TYPE

    def __eq__(self, other):
        if not isinstance(other, Token):
            return False

        return self.text == other.text

    def __repr__(self):
        return smart_utf8('Token(start={}, end={}, text={}, grammem={})'.format(self.start, self.end,
                                                                                self.text, self._grammem))

    def iter_tokens(self):
        all_grams = [self._grammem]
        if self._other_grammems:
            all_grams += self._other_grammems
        res = []
        for gr in all_grams:
            res.append(Token(self.sentence, self.text, self.lemma, ' '.join(gr), self.start, self.end))
        return res

    @classmethod
    def extract_tokens(cls, sample):
        if 'wizard' not in sample.annotations:
            logger.warning('Not found "wizard" annotation. Can\'t extract tokens from sample = %s', sample.text)
            return []

        text = ' '.join(sample.tokens)
        markup = sample.annotations['wizard'].markup

        tokens = []
        wiz_morph = markup.get('Morph', [])
        wiz_tokens = markup.get('Tokens', [])
        for m, t in zip(wiz_morph, wiz_tokens):
            if 'Lemmas' in m:
                lemma = m['Lemmas'][0]
                if 'Text' in lemma:
                    start, end = t['BeginChar'], t['EndChar']
                    grammem = lemma.get('Grammems', [''])[0]
                    other_grammems = lemma.get('Grammems', [''])[1:]
                    tokens.append(Token(text, t['Text'], lemma['Text'], grammem, start, end, other_grammems))

        return tokens

    @classmethod
    def from_dict(cls, data):
        return cls(data['sentence'], data['text'], data['lemma'], data['grammem'],
                   data['start'], data['end'], data['other_grammems'])

    def to_dict(self):
        return {
            'sentence': self.sentence,
            'text': self.text,
            'lemma': self.lemma,
            'grammem': self.grammem,
            'start': self.start,
            'end': self.end,
            'other_grammems': [" ".join(grammem) for grammem in self._other_grammems]
            if self._other_grammems else None
        }


class NounPhrase(SyntaxUnitInterface):
    NOUN_PHRASE_TYPE = 'np'

    def __init__(self, sentence, tokens, head=None, text=None, strict=True):
        super(NounPhrase, self).__init__(sentence)
        self._tokens = tokens
        self._head = head
        self._text = text or ' '.join(tok.text for tok in tokens)

        self._check_values_and_set_head(strict=strict)

    def _check_values_and_set_head(self, strict=True):
        if self._head:
            if self._head.pos != 'S':
                raise ValueError('You passed head with pos={}, need S (noun).'.format(self._head.pos))
            return

        any_noun = False
        for token in self.tokens:
            if not isinstance(token, Token):
                raise ValueError('All tokens must be instances of Token class.')
            if token.pos == 'S':
                any_noun = True
                self._head = token
                break

        if not any_noun:
            if strict:
                raise ValueError("NounPhrase without any nouns can not be constructed.")
            else:
                self._head = self.tokens[0]

    @property
    def grammem(self):
        return self._head.grammem

    @property
    def tokens(self):
        return self._tokens

    @property
    def start(self):
        return self.tokens[0].start

    @property
    def end(self):
        return self.tokens[-1].end

    @property
    def gender(self):
        return self._head.gender

    @property
    def pos(self):
        return 'S'

    @property
    def geo(self):
        return self._head.geo

    @property
    def head(self):
        return self._head

    @property
    def sg_pl(self):
        return self._head.sg_pl

    @property
    def anim(self):
        return self._head.anim

    @property
    def text(self):
        return self._text

    @property
    def type(self):
        return self.NOUN_PHRASE_TYPE

    def __eq__(self, other):
        if not isinstance(other, NounPhrase):
            return False
        return (len(self.tokens) == len(other.tokens) and
                all(map(lambda (x, y): x == y, zip(self.tokens, other.tokens))))

    def __repr__(self):
        return smart_utf8('NounPhrase(text="{}", type="{}", tokens=[...])'.format(self.text, self.type))

    @classmethod
    def from_dict(cls, data):
        tokens = [Token.from_dict(token_dict) for token_dict in data['tokens']]
        head = Token.from_dict(data['head']) if data['head'] is not None else None
        return cls(data['sentence'], tokens, head, data['text'])

    def to_dict(self):
        return {
            'tokens': [token.to_dict() for token in self.tokens],
            'sentence': self.sentence,
            'head': None if self.head is None else self.head.to_dict(),
            'text': self.text
        }


class TokenStorage(object):
    def __init__(self, tokens=None):
        self._text2token = {}
        self._tokens = []

        if tokens:
            self._tokens = tokens

        self._initialize()

    def _initialize(self):
        for tok in self.tokens:
            self._text2token[tok.text] = tok

    @classmethod
    def from_text(cls, text):
        return cls.from_sample(Sample.from_string(text))

    @classmethod
    def from_sample(cls, sample):
        return cls(tokens=Token.extract_tokens(sample))

    @property
    def tokens(self):
        return self._tokens

    def get(self, item, default=None):
        return self._text2token.get(item, default)

    def __getitem__(self, item):
        return self._text2token[item]

    def __contains__(self, item):
        return self._text2token.__contains__(item)


@attr.s
class LSPhrase(object):
    start = attr.ib()
    end = attr.ib()
    name = attr.ib()  # string 'AP', 'NPa', 'NPr', etc.
    head = attr.ib()  # instance of Token
    text = attr.ib()


@attr.s
class SyntaxParserResult(object):
    tokens = attr.ib(default=attr.Factory(list))
    phrases = attr.ib(default=attr.Factory(list))


class SyntaxParser(object):
    _REGISTERED_PHRASES_TYPES = frozenset(['NPa', 'NPg', 'NPe', 'NPn', 'NP'])

    _parser = ya_light_syntax.Parser()

    @classmethod
    def _dfs_helper(cls, raw_phrase, token_storage, text, res):
        phrase_text = text[raw_phrase.Begin:raw_phrase.End]

        head = None
        if raw_phrase.Head == -1:  # raw_phrase is token or light syntax didn't found its head.
            head = token_storage.get(phrase_text)
        else:
            children = list(raw_phrase.GetChildren().iterator())
            for i, child in enumerate(children):
                if i == raw_phrase.Head:  # Head for raw_phrase can be found in this child.
                    head = cls._dfs_helper(child, token_storage, text, res)
                else:
                    cls._dfs_helper(child, token_storage, text, res)

        res.append(LSPhrase(start=raw_phrase.Begin, end=raw_phrase.End,
                            name=raw_phrase.Name, head=head, text=phrase_text))
        return head

    @classmethod
    def _parse_ls_phrases(cls, text, token_storage):
        res = []

        root_phrases = cls._parser.Parse(text)
        for phrase in root_phrases:
            cls._dfs_helper(phrase, token_storage, text, res)

        return res

    @classmethod
    def _create_phrase(cls, text, tokens, head, phrases):
        try:
            phrases.append(NounPhrase(text, tokens, head))
        except ValueError:
            pass

    @classmethod
    def parse(cls, sample):
        token_storage = TokenStorage.from_sample(sample)

        # This step is required because Wizard sometimes breaks multitoken word into two tokens.
        # E.g. 'тель-авив' -> 'тель', 'авив'
        repaired_text = ' '.join([t.text for t in token_storage.tokens])

        ls_phrases = filter(lambda x: x.name in cls._REGISTERED_PHRASES_TYPES,
                            cls._parse_ls_phrases(repaired_text, token_storage))
        tokens = []
        phrases = []
        for token in token_storage.tokens:
            for tok in token.iter_tokens():  # Iterate over tokens with different grammems.
                tokens.append(tok)

        for ls_phrase in ls_phrases:
            try:
                ph_tokens = [token_storage[toktext] for toktext in ls_phrase.text.split()]
            except KeyError:
                raise SyntaxParsingException

            if ls_phrase.head:
                for tok in ls_phrase.head.iter_tokens():
                    cls._create_phrase(repaired_text, ph_tokens, tok, phrases)
            else:
                for comb in all_chains([tok.iter_tokens() for tok in ph_tokens]):
                    cls._create_phrase(repaired_text, comb, None, phrases)

        return SyntaxParserResult(tokens=tokens, phrases=phrases)

    def __call__(self, text):
        return self.parse(text)
