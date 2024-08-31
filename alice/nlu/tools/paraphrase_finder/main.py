# -*- coding: utf-8 -*-

import argparse
import datetime
import getpass
import json
import logging
import nn_applier
import numpy as np
import os
import re
import string
import uuid
import yt.wrapper as yt

from collections import Counter
from copy import deepcopy
from functools import wraps
from yandex_lemmer import AnalyzeWord

logger = logging.getLogger(__name__)


_MODEL_PATH = 'data/boltalka_dssm'
_IDFS_PATH = '//home/voice/dan-anastasev/alice_requests_data/lemma_idf.json'

_TOKENIZE_PATTERN = re.compile(u'([\w]+)', re.U)


def take_last(func):
    @wraps(func)
    def wrapper(*args, **kwargs):
        last_value = None
        for val in func(*args, **kwargs):
            last_value = val
        return last_value
    return wrapper


def _get_input_tables(start_date, end_date):
    start_date = datetime.datetime.strptime(start_date, '%Y-%m-%d').date()
    end_date = datetime.datetime.strptime(end_date, '%Y-%m-%d').date()

    input_dir = '//home/voice/dialog/sessions'
    return [input_dir + '/' + date for date in yt.list(input_dir)
            if start_date <= datetime.datetime.strptime(date, '%Y-%m-%d').date() <= end_date]


class SessionCollector(object):
    def __init__(self, intents_to_drop, min_session_len, max_session_len, max_time_between_turns):
        self._intents_to_drop = intents_to_drop
        self._min_session_len = min_session_len
        self._max_session_len = max_session_len
        self._max_time_between_turns = max_time_between_turns

    def _should_drop_turn(self, turn):
        return any(intent in turn['intent'] for intent in self._intents_to_drop)

    def _is_timedelta_acceptable(self, session, turn_index, subsession_begin):
        time_delta = 0
        if turn_index > subsession_begin:
            time_delta = session[turn_index]['ts'] - session[turn_index - 1]['ts']

        return time_delta <= self._max_time_between_turns

    def __call__(self, input_row):
        def build_output(begin, end):
            if end - begin >= self._min_session_len:
                output_row = deepcopy(input_row)
                output_row['session'] = session[begin: end]
                output_row['session_id'] = '{}_{}_{}'.format(
                    output_row['session_id'], session[begin]['req_id'], session[end - 1]['req_id']
                )
                return output_row

        if input_row['lang'] != 'ru-RU':
            return

        session = input_row['session']

        subsession_begin = 0
        for turn_index, turn in enumerate(session):
            should_drop = self._should_drop_turn(turn)
            is_timedelta_acceptable = self._is_timedelta_acceptable(session, turn_index, subsession_begin)

            subsession_len = turn_index - subsession_begin
            if not should_drop and is_timedelta_acceptable and subsession_len <= self._max_session_len:
                continue

            output_row = build_output(subsession_begin, turn_index)
            if output_row:
                yield output_row

            subsession_begin = turn_index + 1 if should_drop else turn_index

        output_row = build_output(subsession_begin, len(session))
        if output_row:
            yield output_row


def _alice_dssm_applier_preprocessing(text):
    """
    Preprocessing that mimics behaviour for DSSM models on Nirvana.
    Based on:
    https://a.yandex-team.ru/arc/trunk/arcadia/alice/boltalka/libs/text_utils/utterance_transform.h?rev=4674509#L63
    """
    # 1. To lower case
    text = text.lower()
    # 2. Surrounding Python punctuation characters with spaces
    text = re.sub(r'([{}])'.format(re.escape(string.punctuation)), r' \1 ', text)
    # 3. Trimming the text and reducing consequent spaces to one space
    text = ' '.join(text.split())
    # 4. Removing first Alice utterances from the beginning and ending of the text
    alices = (u'алиса', u'алис')
    for prefix in alices:
        if text.startswith(prefix + ' '):
            text = text[len(prefix) + 1:]
            break
    for suffix in alices:
        if text.endswith(' ' + suffix):
            text = text[:-(len(suffix) + 1)]
            break
    return text


def _naive_tokenize(phrase):
    return [tok for tok in _TOKENIZE_PATTERN.findall(phrase) if tok.strip()]


def _get_lemma(text):
    parses = sorted(AnalyzeWord(text, langs=['ru', 'en']), key=lambda x: x.Weight * (x.Last - x.First), reverse=True)
    if not parses:
        return text
    return parses[0].Lemma


def _get_dialog_history(session, begin, end):
    dialog_history = []
    for paraphrased_turn in session[begin: end]:
        dialog_history.append(paraphrased_turn['_query'])
        dialog_history.append(paraphrased_turn['_reply'])

    return dialog_history


class SessionScorer(object):
    def __init__(self, dssm_coef, min_similarity, intents_to_ignore):
        self._model = None
        self._idfs = None
        self._default_idf = None

        self._dssm_coef = dssm_coef
        self._min_similarity = min_similarity
        self._intents_to_ignore = intents_to_ignore

    def start(self):
        with open(os.path.basename(_IDFS_PATH)) as f:
            self._idfs = json.load(f)
            self._default_idf = max(self._idfs.values())
        self._model = nn_applier.Model(os.path.basename(_MODEL_PATH))

    def _apply_dssm(self, text):
        text = text or ''
        text = _alice_dssm_applier_preprocessing(text.decode('utf8')).encode('utf8')
        return np.array(self._model.predict({'context_0': text}, ['query_embedding']))

    def _calc_session_dssm_similarities(self, session):
        embeddings = np.stack([self._apply_dssm(turn['_query']) for turn in session], axis=0)
        similarities = embeddings.dot(embeddings.T)

        dssm_similarities = []
        for turn_index, turn in enumerate(session):
            dssm_similarities.append(similarities[turn_index])

        return dssm_similarities

    def _get_intersection_similarity(self, phrase1, phrase2):
        def _get_normalization_const(token_counts):
            return sum(
                self._idfs.get(token, self._default_idf) * count for token, count in token_counts.most_common()
            )

        if not phrase1 or not phrase2:
            return 0.

        lemmas1 = list(map(_get_lemma, _naive_tokenize(phrase1.decode('utf8'))))
        lemmas2 = list(map(_get_lemma, _naive_tokenize(phrase2.decode('utf8'))))

        if len(lemmas1) == 0 or len(lemmas2) == 0:
            return 0.

        lemma1_counts = Counter(lemmas1)
        lemma2_counts = Counter(lemmas2)

        intersection = 0.
        for lemma, count in lemma1_counts.iteritems():
            lemma_idf = self._idfs.get(lemma, self._default_idf)
            intersection += lemma_idf * min(lemma2_counts[lemma], count)

        lemmas1_normalization_const = _get_normalization_const(lemma1_counts)
        lemmas2_normalization_const = _get_normalization_const(lemma2_counts)

        return 0.5 * (intersection / lemmas1_normalization_const + intersection / lemmas2_normalization_const)

    def _calc_session_intersection_similarities(self, session):
        intersection_similarities = []

        for turn in session:
            intersection_similarities.append([
                self._get_intersection_similarity(turn['_query'], other_turn['_query']) for other_turn in session
            ])

        return intersection_similarities

    def _calc_session_similarities(self, session):
        dssm_similarities = self._calc_session_dssm_similarities(session)
        intersection_similarities = self._calc_session_intersection_similarities(session)

        assert len(dssm_similarities) == len(intersection_similarities)

        similarities = []
        for turn_index in xrange(len(session)):
            turn_dssm_similarities = dssm_similarities[turn_index]
            turn_intersection_similarities = intersection_similarities[turn_index]

            assert len(turn_dssm_similarities) == len(turn_intersection_similarities)

            similarities.append([
                self._dssm_coef * turn_dssm_similarities[i]
                + (1. - self._dssm_coef) * turn_intersection_similarities[i]
                for i in xrange(len(turn_dssm_similarities))
            ])

        return similarities

    @take_last
    def _get_paraphrased_sessions(self, input_row, turn_index, similarities):
        session = input_row['session']
        turn = session[turn_index]
        if any(intent in turn['intent'] for intent in self._intents_to_ignore):
            return

        for next_turn_index in xrange(turn_index + 1, len(similarities)):
            similarity = similarities[next_turn_index]

            if similarity > self._min_similarity and turn['_query'] != session[next_turn_index]['_query']:
                output_row = deepcopy(input_row)

                output_row['session'] = session[turn_index: next_turn_index + 1]
                output_row['dialog_history'] = _get_dialog_history(session, turn_index, next_turn_index + 1)
                output_row['similarity'] = similarity
                output_row['is_end_of_session'] = next_turn_index + 1 == len(session)

                yield output_row

    def __call__(self, input_row):
        session = input_row['session']

        similarities = self._calc_session_similarities(session)

        full_dialog_history = _get_dialog_history(session, 0, len(session))

        for turn_index in xrange(len(session)):
            longest_paraphrased_session = self._get_paraphrased_sessions(
                input_row, turn_index, similarities[turn_index]
            )

            if longest_paraphrased_session:
                longest_paraphrased_session['full_dialog_history'] = full_dialog_history
                longest_paraphrased_session['full_session'] = session
                yield longest_paraphrased_session


def main():
    yt.config['proxy']['url'] = 'hahn'

    logging.basicConfig(level=logging.INFO, format='[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s')

    parser = argparse.ArgumentParser()
    parser.add_argument('--start-date', help='Inclusive, in format %Y-%m-%d')
    parser.add_argument('--end-date', help='Inclusive, in format %Y-%m-%d')
    parser.add_argument('--output-table', help='Path to the output table')

    parser.add_argument(
        '--intents-to-drop', default=['external_skill'], help='Drops such intents and splits sessions by them'
    )
    parser.add_argument('--min-session-len', default=3, help='Ignores sessions shorter than that')
    parser.add_argument('--max-session-len', default=30, help='Splits sessions longer that that')
    parser.add_argument(
        '--max-time-between-turns', default=20, help='Combines turns into session with time delta less that that'
    )
    parser.add_argument(
        '--intents-to-ignore', default=['quasar', 'player', 'sound', 'cancel'], help='Ignores such intents'
    )
    parser.add_argument(
        '--dssm-coef', default=0.7,
        help='Calculates similarity as dssm-coef * dssm-similarity + (1 - dssm-coef) * intersection-similarity'
    )
    parser.add_argument(
        '--min-similarity', default=0.85, help='Threshold for similarity, ignores pairs with lower similarity'
    )
    args = parser.parse_args()

    input_tables = _get_input_tables(args.start_date, args.end_date)

    sessions_table = '//tmp/{}/{}'.format(getpass.getuser(), uuid.uuid4())

    if yt.exists(sessions_table):
        yt.remove(sessions_table)
    yt.create('table', sessions_table, recursive=True)

    session_collector = SessionCollector(
        intents_to_drop=args.intents_to_drop,
        min_session_len=args.min_session_len,
        max_session_len=args.max_session_len,
        max_time_between_turns=args.max_time_between_turns
    )
    yt.run_map(
        session_collector,
        input_tables,
        sessions_table,
        memory_limit=4 * 1024 ** 3,
        job_io={'table_writer': {'max_row_weight' : 128 * 1024 * 1024}},
    )

    session_scorer = SessionScorer(
        dssm_coef=args.dssm_coef,
        min_similarity=args.min_similarity,
        intents_to_ignore=args.intents_to_ignore
    )
    yt.run_map(
        session_scorer,
        sessions_table,
        args.output_table,
        yt_files=[_IDFS_PATH],
        local_files=[_MODEL_PATH],
        memory_limit=16 * 1024 ** 3,
        job_io={'table_writer': {'max_row_weight' : 128 * 1024 * 1024}},
    )


if __name__ == "__main__":
    main()
