# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import attr
import logging

from vins_core.nlu.syntax import SyntaxUnitInterface, SyntaxParser, Token, NounPhrase, SyntaxParsingException
from vins_core.utils.strings import smart_utf8

logger = logging.getLogger(__name__)


class Mention(SyntaxUnitInterface):
    PRONOUN_TYPE = Token.PRONOUN_TYPE
    NOUN_TYPE = Token.NOUN_TYPE
    OTHER_TYPE = Token.OTHER_TYPE
    NOUN_PHRASE_TYPE = NounPhrase.NOUN_PHRASE_TYPE

    SYNTAX_SOURCE_TYPE = 'syntax'
    ES_SOURCE_TYPE = 'entitysearch'

    _syntax_parser = SyntaxParser()

    def __init__(self, m_value, source=SYNTAX_SOURCE_TYPE):
        """
        Args:
            m_value (syntax.SyntaxUnitInterface): Mention value. Token or NounPhrase.
            source (basestring, optional): Source mention comes from. Could be one of:
                * "syntax" -- mention obtained from user query with syntax parser.
                * "entitysearch" -- mention obtained from EntitySearchAnnotation of user query.
                Default is "syntax".
        """
        super(Mention, self).__init__(m_value.sentence)
        self._value = m_value
        self._source = source

        assert self._source in [self.SYNTAX_SOURCE_TYPE, self.ES_SOURCE_TYPE]

    @property
    def source(self):
        return self._source

    @property
    def grammem(self):
        return self._value.grammem

    @property
    def type(self):
        return self._value.type

    @property
    def start(self):
        return self._value.start

    @property
    def end(self):
        return self._value.end

    @property
    def gender(self):
        return self._value.gender

    @property
    def pos(self):
        return self._value.pos

    @property
    def geo(self):
        return self._value.geo

    @property
    def sg_pl(self):
        return self._value.sg_pl

    @property
    def head(self):
        return self._value.head

    @property
    def anim(self):
        return self._value.anim

    @property
    def text(self):
        return self._value.text

    @classmethod
    def _filter_syntax_mentions(cls, mentions):
        filtered_mentions = {}
        for mention in mentions:
            key = (mention.start, mention.end, mention.text, mention.pos,
                   mention.geo, mention.gender, mention.anim, mention.sg_pl)
            if key not in filtered_mentions:
                filtered_mentions[key] = mention
        return filtered_mentions.itervalues()

    @classmethod
    def _parse_syntax_mentions(cls, sample):
        try:
            parser_result = cls._syntax_parser.parse(sample)
        except SyntaxParsingException:
            logger.warning('Syntax parser failed to parse sample "%s". No mentions to return.', sample.text)
            return []

        mentions = []
        mentions.extend(parser_result.phrases)

        # Filter out unnecessary tokens.
        mentions.extend(filter(lambda x: x.type != cls.OTHER_TYPE, parser_result.tokens))
        mentions = cls._filter_syntax_mentions(mentions)
        return [Mention(val, Mention.SYNTAX_SOURCE_TYPE) for val in mentions]

    @classmethod
    def parse_entity_search_mentions(cls, sample):
        mentions = []

        sample_tokens = Token.extract_tokens(sample)
        sentence = ' '.join([t.text for t in sample_tokens])
        if 'entitysearch' in sample.annotations:
            entities = sample.annotations['entitysearch'].entities
            for ent in entities:
                start_token, end_token = ent.start, ent.end
                ent_tokens = sample_tokens[start_token:end_token]

                val = NounPhrase(sentence, ent_tokens, text=ent.name, strict=False)
                source_type = Mention.ES_SOURCE_TYPE

                mentions.append(Mention(val, source_type))

        return mentions

    @classmethod
    def _parse_previous_match_mentions(cls, sample):
        mentions = []
        if 'anaphora_matched_mention' in sample.annotations:
            mentions.append(sample.annotations['anaphora_matched_mention'].antecedent_mention)
        return mentions

    @classmethod
    def parse_mentions(cls, sample, with_entitysearch=False, with_syntax=True):
        """Parses all mentions in `sample` and returns them in order they go in the text.

        Args:
            sample (vins_core.common.sample.Sample): Original sample.
            with_entitysearch (bool, optional): Whether to create Mentions from EntitySearchAnnotation entities or not.
            with_syntax (bool, optional): Whether to create Mentions from syntax parser result or not.

        Returns:
            mentions (list of Mention): List of mentions.
        """
        logger.debug('Parsing metions from sample {}'.format(sample.text))
        if not sample:
            return []
        mentions = []

        if with_syntax:
            mentions += cls._parse_syntax_mentions(sample)

        if with_entitysearch:
            mentions += cls.parse_entity_search_mentions(sample)

        mentions += cls._parse_previous_match_mentions(sample)

        res = sorted(mentions, key=lambda x: x.start)
        logger.debug('Mention.parse_mentions("%s") = %s', sample.text, repr(res).decode('utf8'))
        return res

    @classmethod
    def from_dict(cls, data):
        if data['type'] == 'np':
            value = NounPhrase.from_dict(data['value'])
        elif data['type'] in ['pronoun', 'noun', 'other']:
            value = Token.from_dict(data['value'])
        else:
            raise ValueError('Only token and noun phrase can be values in Mention')
        return cls(value, data['source'])

    def to_dict(self):
        return {'value': self._value.to_dict(), 'source': self._source, 'type': self.type}

    def __eq__(self, other):
        return self._value == other._value

    def __repr__(self):
        return smart_utf8('Mention(type={}, text={})'.format(self.type, self.text))


@attr.s
class Match(object):
    anaphor = attr.ib()
    antecedent = attr.ib()
    score = attr.ib(default=0.0)
