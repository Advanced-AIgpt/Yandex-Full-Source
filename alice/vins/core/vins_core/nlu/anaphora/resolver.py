# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import attr
import logging
import operator

from vins_core.common.annotations import AnnotationsBag, BaseAnnotation, register_annotation
from vins_core.common.sample import Sample
from vins_core.utils import lemmer
from vins_core.nlu.anaphora.mention import Mention
from vins_core.nlu.anaphora.annotation_utils import (
    has_anaphora_annotations, rename_response_to_turn_annotations,
    rename_turn_to_response_annotations, extract_anaphora_annotations
)
from vins_core.nlu.anaphora.context import AnaphoricContext

logger = logging.getLogger(__name__)


class AnaphoraResolver(object):
    _lemmer = lemmer.Lemmer(['ru', 'en'])
    _inflector = lemmer.Inflector('ru')

    ADVPRO_TO_CASE = {'там': 'abl', 'здесь': 'abl', 'туда': 'acc', 'сюда': 'acc', 'оттуда': 'gen', 'отсюда': 'gen'}
    ADVPRO_TO_PREPOSITION = {'там': 'в', 'здесь': 'в', 'туда': 'в', 'сюда': 'в', 'оттуда': 'из', 'отсюда': 'от'}

    def __init__(self, matcher, samples_extractor, max_utterances=2, with_entitysearch=False):
        self._matcher = matcher
        self._max_utterances = max_utterances
        self._with_entitysearch = with_entitysearch
        self._samples_extractor = samples_extractor

    def __call__(self, sample, session):
        matches = self.resolve_anaphora(sample, session)
        if matches:
            logger.debug('Matching pronoun "%s"(%s) with phrase "%s"(%s). Score is %f', matches[0].anaphor.text,
                         matches[0].anaphor.grammem, matches[0].antecedent.text,
                         matches[0].antecedent.grammem, matches[0].score)
            resolved_sentence = self._substitute(matches[0])

            match_annotation = AnaphoraMatchedMentionAnnotation(antecedent_mention=matches[0].antecedent)

            if not session.annotations:
                session.annotations = AnnotationsBag()

            session.annotations['anaphora_matched_mention'] = match_annotation

            return resolved_sentence
        return None

    def resolve_anaphora(self, sample, session):
        if not sample or not session or len(session.dialog_history) == 0:
            return None

        context = self._prepare_context(session)

        logger.debug('Anaphoric context extracted from history')

        matches = self._process(sample, context)
        return matches

    def find_suitable_anaphoric_entity(self, slot, sample, session):
        sample_has_pronoun = any(token in slot.import_entity_pronouns for token in sample.tokens)
        if not sample_has_pronoun:
            return None
        context = self._prepare_context(session)
        for sender, sample in reversed(context):
            extracted_entity = self._select_entity_for_slot(slot, sample.annotations.get('entitysearch'))
            if extracted_entity:
                return extracted_entity
        return None

    def _prepare_context(self, session):
        last_phrases = list(session.dialog_history.last_phrases(count=2 * self._max_utterances))
        prepared_context = self._create_samples_from_phrases(last_phrases)
        return prepared_context

    def _create_samples_from_phrases(self, phrases):
        context_samples = []
        for phrase in phrases:
            sample = Sample.from_string(phrase.text, annotations=phrase.annotations)
            if phrase.sender == 'vins':
                rename_turn_to_response_annotations(sample.annotations)
                if not has_anaphora_annotations(sample.annotations):
                    sample = extract_anaphora_annotations(sample, self._samples_extractor, is_response=True)
                    if phrase.annotations is None:
                        phrase.annotations = AnnotationsBag()
                    response_annotations = sample.annotations.copy()
                    rename_response_to_turn_annotations(response_annotations)
                    phrase.annotations.update(response_annotations)
            logger.debug('Sample {} from {} has [{}] annotations'.format(sample.text, phrase.sender,
                                                                         ', '.join(sample.annotations.keys())))
            context_samples.append((phrase.sender, sample))
        return context_samples

    def _process(self, sample, context):
        """Find coreferences between sample and context and return list of `Match` objects.

        Args:
            sample (vins_core.common.sample.Sample): Yet another sentence.
            context (list of tuple(str, vins_core.common.sample.Sample)):
                Context samples [turn_-k, turn_-(k-1), ..., turn_-1].

        Returns:
            list: List of result matches.

        """
        turn_mentions = Mention.parse_mentions(sample, with_entitysearch=self._with_entitysearch, with_syntax=True)

        # No mentions found or errors occured.
        if not turn_mentions:
            return []

        # Find best intra-turn match. If it has greater score than best inter-turn match, then we do not resolve.
        best_intra_match = None
        best_intra_score = 0.0
        for i, mention in enumerate(turn_mentions):
            if mention.type == Mention.PRONOUN_TYPE:
                anaphoric_context = AnaphoricContext(mention, [turn_mentions[:i]],
                                                     same_request_mentions=True, senders=['user'])
                match = self._find_antecedent(anaphoric_context)
                if match is not None and match.score > best_intra_score:
                    best_intra_match = match
                    best_intra_score = match.score

        matches = []
        context_mentions = self._make_context_from_samples(context)
        for mention in turn_mentions:
            if mention.type == Mention.PRONOUN_TYPE:
                anaphoric_context = AnaphoricContext(mention, context_mentions, same_request_mentions=False,
                                                     senders=[sender for sender, _ in context])
                match = self._find_antecedent(anaphoric_context)
                if match:
                    matches.append(match)

        matches = sorted(matches, key=operator.attrgetter('score'), reverse=True)

        # Check if intra-turn match has greater score or not.
        if best_intra_match and matches and best_intra_score >= matches[0].score:
            logger.debug('Best match is intra-turn, so do not resolve.')
            matches = []

        return matches

    def _make_context_from_samples(self, samples):
        context_mentions = []
        for sender, ctx_sample in samples:
            with_syntax = sender == 'user'
            context_mentions.append(Mention.parse_mentions(ctx_sample, with_entitysearch=self._with_entitysearch,
                                                           with_syntax=with_syntax))
        return context_mentions

    def _find_antecedent(self, anaphoric_context):
        """Call `_match()` between `mention` and all candidates and returns one with maximum score.
        If there are two candidates with equal scores, the last one will be returned.
        """
        if not anaphoric_context.antecedents:
            return None

        matches = self._matcher.match(anaphoric_context)
        if not matches:
            return None

        # If there are two or more candidates with equal scores, the last one will be returned
        best_match = max(reversed(matches), key=lambda x: x.score)
        return best_match

    @classmethod
    def _extract_declination_info_from_anaphor(cls, anaphor):
        info = {}
        if anaphor.pos == 'ADVPRO':
            # If mention is 'там', 'туда', 'сюда', 'отсюда', 'оттуда', 'здесь', then we need to properly inflect.
            info['antecedent__target_case'] = cls.ADVPRO_TO_CASE.get(anaphor.text)
            info['antecedent__prepend_with'] = cls.ADVPRO_TO_PREPOSITION.get(anaphor.text)
        return info

    @classmethod
    def _substitute(cls, match):
        """Substitute `match.antecedent` in place of `match.anaphor` in the original sentence.

        Args:
            match (Match): Match object.
        Returns:
            str: Original sentence with resolved coreference.

        """
        anaphor, antecedent = match.anaphor, match.antecedent

        original_sentence = anaphor.sentence
        raw_match_text = original_sentence[anaphor.start:anaphor.end]
        raw_antecedent_text = antecedent.text

        # Try to inflect mention `antecedent` to proper case.
        declination_info = cls._extract_declination_info_from_anaphor(anaphor)
        try:
            target_case = (declination_info.get('antecedent__target_case') or
                           cls._lemmer.parse(raw_match_text)[0].tag.case)
            inflected_antecedent_text = cls._inflector.inflect(raw_antecedent_text, [target_case])
        except Exception:
            inflected_antecedent_text = raw_antecedent_text

        if declination_info.get('antecedent__prepend_with'):
            inflected_antecedent_text = declination_info['antecedent__prepend_with'] + ' ' + inflected_antecedent_text

        return original_sentence[:anaphor.start] + inflected_antecedent_text + original_sentence[anaphor.end:]

    @classmethod
    def _select_entity_for_slot(cls, slot, entity_annotation):
        if not entity_annotation:
            return None
        for entity in entity_annotation.entities:
            has_common_tags = (len(slot.import_entity_tags.intersection(set(entity.tags))) > 0)
            slot_allows_entity_type = any(map(entity.type.startswith, slot.import_entity_types))

            if has_common_tags or slot_allows_entity_type:
                return entity
        return None


@attr.s
class AnaphoraMatchedMentionAnnotation(BaseAnnotation):
    antecedent_mention = attr.ib()

    def to_dict(self):
        return {'antecedent_mention': self.antecedent_mention.to_dict()}

    @classmethod
    def from_dict(cls, data):
        return cls(antecedent_mention=Mention.from_dict(data.get('antecedent_mention')))


register_annotation(AnaphoraMatchedMentionAnnotation, 'anaphora_matched_mention')
