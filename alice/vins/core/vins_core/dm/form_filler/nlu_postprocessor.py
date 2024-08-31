# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import datetime
import copy
import logging
import operator
import re

from pymorphy2 import MorphAnalyzer
from pymorphy2.units.by_analogy import KnownSuffixAnalyzer
import pandas as pd
import numpy as np
from itertools import izip, groupby

from vins_core.dm.frame_scoring.base import FrameScoring
from vins_core.dm.form_filler.models import SlotConcatenation
from vins_core.logger.utils import dump_sequence
from vins_core.utils.lemmer import Inflector
from vins_core.utils.misc import EPS
from vins_core.utils.operators import item
from vins_core.common.slots_map_utils import merge_slots

logger = logging.getLogger(__name__)


class NluPostProcessor(object):
    """
    Nlu post processor object takes iterable of semantic frames
    and returns modified one
    """
    def __call__(self, frames, session, **kwargs):
        out_frames = self._call(frames, session, **kwargs)
        return out_frames

    def _call(self, frames, session, **kwargs):
        raise NotImplementedError()


class ChainedNluPostProcessor(NluPostProcessor):
    def __init__(self, nlu_post_processors):
        self._nlu_post_processors = nlu_post_processors

    def _call(self, frames, session, **kwargs):
        for nlu_post_processor in self._nlu_post_processors:
            frames = nlu_post_processor(frames, session, **kwargs)
        return frames


class DatetimeNowNluPostProcessor(NluPostProcessor):

    _MOD_TYPE = 'DATETIME'

    def _call(self, frames, session, **kwargs):
        out_frames = copy.deepcopy(frames)
        client_time = kwargs.get('client_time')
        if not client_time:
            logger.warning('Datetime should be modified with client_time, but "client_time" is not provided in kwargs')
            return out_frames
        for nlu_map in out_frames:
            self._apply_datetime_client_time(nlu_map['entities'], client_time)
            for slot_name, slot_values in self._iter_slots(nlu_map, kwargs.get('forms_map')):
                for slot_partial_value in slot_values:
                    self._apply_datetime_client_time(slot_partial_value['entities'], client_time)
        return out_frames

    @classmethod
    def _iter_slots(cls, nlu_map, forms_map):
        if forms_map and forms_map.get(nlu_map['intent_name']):
            modify_slot = {s.name: cls._MOD_TYPE.lower() in s.types
                           for s in forms_map[nlu_map['intent_name']].slots}
            for slot_name, slot_value in nlu_map['slots'].iteritems():
                if modify_slot[slot_name]:
                    yield slot_name, slot_value

    @classmethod
    def _fix_rel_units(cls, rel_units, v):
        out = {'days': 0}
        for k in rel_units:
            if k == 'years':
                out['days'] += 365 * v['years']
            elif k == 'months':
                out['days'] += 30 * v['months']
            elif k == 'weeks':
                out['days'] += 7 * v['weeks']
            else:
                out[k] = v[k]
        return out

    @staticmethod
    def _cut_relative(string):
        relative = '_relative'
        if string.endswith(relative):
            string = string[:-len(relative)]
        return string

    def _apply_datetime_client_time(self, entities, client_time):
        for e in entities:
            if e.type != self._MOD_TYPE:
                continue
            v = e.value

            rel_units = [self._cut_relative(k) for k in v if k.endswith('_relative')]
            if 'time' in rel_units:
                rel_units.remove('time')
                rel_units.extend(['seconds', 'minutes', 'hours'])
            if 'date' in rel_units:
                rel_units.remove('date')
                rel_units.extend(['days', 'months', 'years'])

            td_args = self._fix_rel_units(rel_units, v)
            try:
                td = datetime.timedelta(**td_args)
                if 'weekday' in v:
                    # resolving week days when current weekday (client_wd) is specified
                    client_wd = client_time.weekday()
                    td_days = 0
                    if client_wd == v['weekday'] and td.days == 7:  # one week later
                        td_days = 7
                    elif client_wd > v['weekday'] and td.days >= 0:  # next following weekday, next week
                        td_days = 7 - (client_wd - v['weekday'])
                    elif client_wd < v['weekday'] and td.days >= 0:  # next following weekday, this week
                        td_days = v['weekday'] - client_wd
                    elif td.days == -7:  # last week
                        td_days = v['weekday'] - client_wd - 7

                    del v['weekday']
                    td += datetime.timedelta(days=td_days - td.days)

                dt = client_time + td
            except OverflowError:
                if any(t < 0 for t in td_args.itervalues()):
                    dt = datetime.datetime.min
                elif any(t > 0 for t in td_args.itervalues()):
                    dt = datetime.datetime.max
                else:
                    raise

            v = {
                'seconds': dt.second,
                'minutes': dt.minute,
                'hours': dt.hour,
                'days': dt.day,
                'months': dt.month,
                'years': dt.year
            }
            e.value.update(
                {kk: vv for kk, vv in v.iteritems() if not (kk in e.value and kk not in rel_units)}
            )
            e.value = {k: v for k, v in e.value.iteritems() if not k.endswith('_relative')}


class ConditionalSlotNormalizer(object):
    _morph_analyser = MorphAnalyzer()

    @classmethod
    def normalize_slot_value(cls, slot_value, sample, normalization_config, inflector):
        if not sample:
            return
        prefix_text = ' '.join(sample.text.split()[:slot_value['start']])
        for source_case_config in normalization_config:
            several_words = source_case_config.get('several_words', False) and len(slot_value['substr'].split()) > 1
            if re.match(source_case_config['prefix'], prefix_text, re.UNICODE):
                if several_words:
                    slot_value['substr'] = cls._inflect_text_from_case(
                        slot_value['substr'],
                        source_case_config['source_case'],
                        inflector=inflector)
                else:
                    slot_value['substr'] = cls._inflect_word_from_case(
                        slot_value['substr'],
                        source_case_config['source_case'])
                break

    @classmethod
    def _check_parse_suitability(cls, parse, source_case):
        return (source_case in parse.tag and 'NOUN' in parse.tag
                and not any(isinstance(unit[0], KnownSuffixAnalyzer.FakeDictionary) for unit in parse.methods_stack))

    @classmethod
    def _inflect_word_from_case(cls, text, source_case):
        # Normalize only single nouns
        if len(text.split()) > 1:
            return text
        parses = cls._morph_analyser.parse(text)
        # Prefer single to plural number
        parses.sort(key=lambda x: (x.score, x.tag.number == 'sing'), reverse=True)
        for parse in parses:
            if cls._check_parse_suitability(parse, source_case):
                logger.debug("%s normalized to  %s", text, parse.inflect({'nomn'}).word)
                return parse.inflect({'nomn'}).word
        return text

    @classmethod
    def _inflect_text_from_case(cls, text, source_case, inflector):
        words = text.split()
        words_parse = [cls._morph_analyser.parse(word)[0] for word in words]
        if any(cls._check_parse_suitability(parse, source_case) for parse in words_parse):
            return inflector.inflect(text, ["nomn"])
        return text


class NormalizingNluPostProcessor(NluPostProcessor):
    """
    For slots marked with `normalize_to` field, converts value
    to the specified normal form.

    NOTE: applies to non-entity slots (strings) only!
    """

    _inflector = Inflector('ru')

    def __init__(self):
        self._conditional_normalizer = ConditionalSlotNormalizer()

    def _call(self, frames, session, **kwargs):
        out_frames = copy.deepcopy(frames)
        self._apply_uncoditional_normalization(out_frames, kwargs.get('forms_map'))
        self._apply_conditional_normalization(out_frames, kwargs.get('forms_map'), kwargs.get('sample'))
        return out_frames

    def _apply_uncoditional_normalization(self, frames, forms_map):
        for nlu_map in frames:
            for slot_value, normalize_to in self._iter_slots(nlu_map, forms_map, normalize_to=True):
                self._apply_normalization(slot_value, normalize_to)

    def _apply_conditional_normalization(self, frames, forms_map, sample):
        for nlu_map in frames:
            for slot_value, normalization_config in self._iter_slots(nlu_map, forms_map, normalize_from=True):
                self._conditional_normalizer.normalize_slot_value(
                    slot_value, sample, normalization_config,
                    self.__class__._inflector)

    @classmethod
    def _iter_slots(cls, nlu_map, forms_map, normalize_to=False, normalize_from=False):
        assert(normalize_to ^ normalize_from)
        if forms_map:
            form = forms_map.get(nlu_map['intent_name'])
            if form:
                for slot in form.slots:
                    slot_values = nlu_map['slots'].get(slot.name)
                    if normalize_to and slot_values and slot.normalize_to:
                        for slot_partial_value in slot_values:
                            if not slot_partial_value.get('entitites'):
                                yield slot_partial_value, slot.normalize_to
                    elif normalize_from and slot_values and slot.prefix_normalization:
                        for slot_partial_value in slot_values:
                            if not slot_partial_value.get('entitites'):
                                yield slot_partial_value, slot.prefix_normalization

    @classmethod
    def _apply_normalization(cls, slot_value, normalize_to):
        words = slot_value['substr'].split(' ')
        slot_value['substr'] = ' '.join([cls._inflector.inflect(word, normalize_to) for word in words])


class TopSelectorNluPostProcessor(NluPostProcessor):
    """
    Select one frame with greatest confidence
    """
    def _call(self, frames, session, **kwargs):
        if not frames:
            return []
        return [max(frames, key=operator.itemgetter('confidence'))]


class SortNluPostProcessor(NluPostProcessor):
    """
    Sort input frames by specified key
    """
    def __init__(self, key='confidence'):
        self._key = key

    def _call(self, frames, session, **kwargs):
        key = self._key
        if isinstance(key, (str, unicode)):
            key = operator.itemgetter(key)
        frames = copy.copy(frames)
        frames.sort(key=key, reverse=True)
        logger.debug('Reranked frames:\n%s', dump_sequence(
            [{'name': frame['intent_name'],
              'confidence': frame['confidence']} for frame in frames], ['confidence', 'name']))
        return sorted(frames, key=key, reverse=True)


class CutoffNluPostProcessor(NluPostProcessor):
    """
    Removes all frames where confidence is equal or less than specified cutoff value
    """
    def __init__(self, cutoff=0., dont_kill_all_frames=True):
        """
        :param cutoff: confidence threshold value
        :param dont_kill_all_frames: if True and all confidences are bellow threshold,
                                    then empty frame with first intent is returned
        """
        self._cutoff = cutoff
        self._dont_kill_all_frames = dont_kill_all_frames

    def _call(self, frames, session, **kwargs):
        result_frames = filter(item('confidence') > self._cutoff, frames)
        if len(result_frames) == 0 and self._dont_kill_all_frames:
            result_frames = [dict(
                slots={},
                entities={},
                intent_candidate=frames[0].get('intent_candidate'),
                intent_name=frames[0]['intent_name'],
                confidence=frames[0]['confidence'],
                tagger_score=None
            )]
        return result_frames


class RescoringNluPostProcessor(NluPostProcessor):
    """
    Applies given rescoring models in parallel to each given frame
    and reduces results using specified algorithm
    """

    _REDUCERS = ('prod', 'mean')

    def __init__(self, frame_scoring_models, reducer='prod', normalize=True):
        """
        :param frame_scoring_models: iterable of vins_core.dm.frame_scoring.base.FrameScoring
        :param reducer: method name to reduce scores:
                - 'mean': average scores
                - 'prod': multiply scores (by default)
        :return:
        """
        assert all(isinstance(m, FrameScoring) for m in frame_scoring_models)
        assert reducer in self._REDUCERS
        self._frame_scoring_models = frame_scoring_models
        self._reducer = reducer
        self._normalize = normalize

    def _call(self, frames, session, **kwargs):
        scores = pd.DataFrame(
            data=np.asarray(
                map(lambda m: m(frames, session, **kwargs), self._frame_scoring_models),
                dtype=np.float32
            ).T,
            columns=map(operator.attrgetter('__class__.__name__'), self._frame_scoring_models),
            index=map(lambda x: x['intent_name'], frames)
        ).dropna(axis=1, how='all')
        if not scores.empty:
            if self._reducer == 'prod':
                scores = scores.prod(axis=1)
            elif self._reducer == 'mean':
                scores = scores.mean(axis=1)
            else:
                raise ValueError('Reducer method is not recognized. Should be one of: {0}'.format(self._REDUCERS))
            if self._normalize:
                scores /= max(scores.sum(), EPS)
            for frame, score in izip(frames, scores):
                frame['confidence'] = float(score)

        return frames


class ContinuedSlotNluPostProcessor(NluPostProcessor):
    """
    Merges multiple slots in case of slot continuation.
    E.g. in '"смешные"(search_text) "видео"(content_type) "котов"(+search_text)' search_text will be merged.
    Force slot merge even with B- tags if this is indicated in slot config
    """
    def _call(self, frames, session, **kwargs):
        forms_map = kwargs.get('forms_map')
        if not forms_map:
            logger.warning('Forms map should be provided, but "forms_map" is not provided in kwargs')
            return frames
        for frame in frames:
            form = forms_map.get(frame['intent_name'])
            if not form:
                logger.warning('Form map for intent "{}" is not provided, frame skipped'.format(
                    frame['intent_name']))
                continue
            matching_types = {s.name: s.matching_type for s in form.slots}
            for slot_name, slot_value in frame['slots'].iteritems():
                if len(slot_value) == 1:
                    # nothing to concatenate, if there are no multiple occurrences of a slot
                    continue
                form_slot = form.get_slot_by_name(slot_name)
                if (not form_slot or 'string' not in form_slot.types or
                        form_slot.concatenation == SlotConcatenation.forbid):
                    # nothing to concatenate, if concatenation is not allowed or values are not strings
                    continue
                slot_value = sorted(slot_value, key=operator.itemgetter('start'))
                new_slot_value = []

                # merge each 2 consecutive slot values if the second one is a continuation (starts with I- tag)
                prev_slot = slot_value[0]
                for next_slot in slot_value[1:]:
                    if next_slot.get('is_continuation', False) or (form_slot.concatenation == SlotConcatenation.force):
                        prev_slot = merge_slots(prev_slot, next_slot, matching_types.get(slot_name, 'exact'))
                    else:
                        new_slot_value.append(prev_slot)
                        prev_slot = next_slot
                new_slot_value.append(prev_slot)

                frame['slots'][slot_name] = new_slot_value
        return frames


class FirstFrameChoosingNluPostProcessor(NluPostProcessor):
    def _call(self, frames, session, **kwargs):
        for frame in frames:
            return [frame]
        return []


class IntentGroupNluPostProcessor(NluPostProcessor):
    def __init__(self, group_post_processor):
        self._group_post_processor = group_post_processor

    def _call(self, frames, session, **kwargs):
        result_frames = []
        for intent_name, intent_frames in groupby(frames, operator.itemgetter('intent_name')):
            result_frames.extend(self._group_post_processor(list(intent_frames), session, **kwargs))
        return result_frames
