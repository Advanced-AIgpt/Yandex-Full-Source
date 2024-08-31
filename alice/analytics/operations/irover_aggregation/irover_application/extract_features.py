# -*- coding: utf8 -*-

import argparse
import json
import logging
import sys
from collections import defaultdict, Counter
from re import findall, compile

sys.path.append('..')

from utils.word_transition_network import *


def setup_logger():
    root_logger = logging.getLogger('')
    file_logger = logging.StreamHandler()
    file_logger.setFormatter(logging.Formatter('%(asctime)s %(levelname)-8s %(filename)s:%(lineno)d %(message)s'))
    root_logger.addHandler(file_logger)
    root_logger.setLevel(logging.DEBUG)


def parse_cmd_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('-i', '--input', dest='input', help='json with assignments - sb urls', required=True)
    parser.add_argument('--lang', metavar='LANG', help='input language (ru|tr)', required=True)
    parser.add_argument('--field', help='text field for feature extracting',
                        required=True)
    parser.add_argument('--platform', metavar='PLATFORM', help='name of the platform, e.g. \"toloka\"/\"yang\"',
                        required=True)
    parser.add_argument('--max_num_answers', metavar='MAX_NUM_ANSWERS', help='Max number of assignments for one record '
                                                                             'from the pool',
                        type=int,
                        required=True)
    parser.add_argument('--for_fitting', help='Should extract features for all costs for fitting? default: False',
                        action="store_true")
    parser.add_argument('--rows', dest='rows', help='rows for evaluation', required=True)
    parser.add_argument('--features', dest='features', help='features of rows to evaluate', required=True)
    context = parser.parse_args(sys.argv[1:])

    return context


def write_result(name, result, is_json_list=False):
    logging.debug("{} results: {}".format(name, len(result)))
    with open(name, 'w') as out:
        if is_json_list:
            for obj in result:
                json.dump(obj, out, ensure_ascii=False)
                out.write('\n')
        else:
            json.dump(result, out, indent=4, ensure_ascii=False)


def process(context):
    assignments = json.loads(open(context.input, encoding="utf8").read())
    logging.debug('load {} assignments'.format(len(assignments)))
    processor = FeaturesExtractor(assignments, context.platform, context.max_num_answers,
                                  context.for_fitting, context.field)
    features = processor.extract_features()
    rows = processor.get_rows()
    write_result(context.rows, rows)
    write_result(context.features, features)


class FeaturesExtractor:
    def __init__(self, assignments, platform, max_num_answers, for_fitting, field):
        self.assignments = assignments
        self.platform = platform
        self.max_num_answers = max_num_answers
        self.for_fitting = for_fitting
        self.NO_SOURCE_ID = 'NO WORKER'
        self.eng_letter = compile("[a-zA-Z]")
        self.field = field

        self.rows = []
        self.features = {}

        # if not self.for_fitting:
        self.prepare_data()

    def prepare_data(self):
        task_answers = defaultdict(lambda: {'raw_assesments': []})
        for answer in self.assignments:
            input_values = answer.get('inputValues')
            if input_values is None:
                input_values = {'audio': answer['url']}
            key = input_values['audio']
            raw_assesments = answer.get('raw_assesments')
            task_answers[key]['inputValues'] = input_values
            task_answers[key]['taskId'] = answer.get('taskId')
            task_answers[key]['poolId'] = answer.get('poolId')
            if raw_assesments is None:
                value = {
                    'platform': self.platform,
                    'submitTs': answer['submitTs'],
                    'workerId': answer['workerId'],
                    'assignmentId': answer['assignmentId']
                }
                value.update(answer['outputValues'].copy())
                value['text'] = value.get(self.field, "")
                if self.field in value:
                    del value[self.field]

                task_answers[key]['raw_assesments'].append(value)
            else:
                raw_assesments_copy = []
                for x in raw_assesments:
                    x = x.copy()
                    x['workerId'] = x['workerId']
                    value['text'] = value.get(self.field, "")
                    x['text'] = x.get(self.field, "")
                    if self.field in x:
                        del x[self.field]
                    raw_assesments_copy.append(x)
                task_answers[key]['raw_assesments'] = raw_assesments_copy
                task_answers[key]['query'] = answer.get('query')
                task_answers[key]['full_text'] = answer.get('full_text')
        for key, value in task_answers.items():
            row = {
                'inputValues': value['inputValues'],
                'raw_assesments': value['raw_assesments'],
                'poolId': value['poolId'],
                'taskId': value['taskId']
            }
            if self.for_fitting:
                row['text'] = value.get('full_text' if self.field == 'annotation' else self.field, '')
            self.rows.append(row)
        logging.debug('prepared {} rows'.format(len(self.rows)))

    def get_rows(self):
        rows_copy = []
        for row in self.rows:
            row_copy = row.copy()
            assesments_copy = []
            for x in row_copy['raw_assesments']:
                x = x.copy()
                x[self.field] = x['text']
                del x['text']
                assesments_copy.append(x)
            row_copy['raw_assesments'] = assesments_copy
            rows_copy.append(row_copy)
        return rows_copy

    @staticmethod
    def majority_vote_text(hyps):
        answers = [hyp.value for hyp in hyps]
        answers = Counter(answers)
        if answers.most_common(1)[0][1] < 2:
            return ''
        else:
            return answers.most_common(1)[0][0]

    def _extract_prefeatures(self):
        prefeatures = collections.defaultdict(lambda: collections.defaultdict(lambda: collections.defaultdict(dict)))
        for row in self.rows:
            toloka_answers = sorted(row['raw_assesments'], key=lambda x: x['submitTs'])
            id_ = row['inputValues']['audio']
            if self.for_fitting:
                cost_range = range(1, min(self.max_num_answers, len(toloka_answers)) + 1)
            else:
                cost_range = [min(self.max_num_answers, len(toloka_answers))]
            for cost in cost_range:
                hyps = []
                for text, worker_id in [(x['text'], x['workerId']) for x in toloka_answers[:cost]]:
                    text = text.lower().replace('ё', 'е')
                    hyps.append(TextHyp(id_, worker_id, text))
                wtn = WordTransitionNetwork(object_id=id_, hypotheses=hyps)
                ref_text = row.get('text', '')
                # ref = WordTransitionNetwork(object_id=id_, hypotheses=[TextHyp(id_, 'reference', ref_text)])
                ref = WordTransitionNetwork(object_id=id_, hypotheses=[TextHyp(id_, 'reference', ref_text)])
                alignment, actions = wtn._align(wtn.edges, ref.edges, wtn.hypotheses_sources, ref.hypotheses_sources)
                expanded_alignment = []  # формируем дополнительные ребра на местах где не было вставок
                skip_next = False
                for item, action in zip(alignment, actions):
                    if action == 'I':
                        if skip_next:
                            continue  # берем только первую вставку из нескольких так как они индентичны
                        expanded_alignment.append(item)
                        skip_next = True
                    elif skip_next:
                        expanded_alignment.append(item)
                        skip_next = False
                    else:
                        expanded_alignment += [
                            {'': WTNEdge('',
                                         None,
                                         wtn.hypotheses_sources + ['reference'],
                                         [None for _ in wtn.hypotheses_sources + ['reference']])}, item
                        ]
                if not skip_next:
                    expanded_alignment.append(
                        {
                            '': WTNEdge(
                                '',
                                None,
                                wtn.hypotheses_sources + ['reference'],
                                [None for _ in wtn.hypotheses_sources + ['reference']]
                            )
                        }
                    )
                for position, edges in enumerate(expanded_alignment):
                    correct_word = None
                    fixed_edges = {}
                    for word, edge in edges.items():
                        if 'reference' in edge.sources:
                            edge = WTNEdge(edge[0], edge[1], edge[2][:-1], edge[3][:-1])
                            assert 'reference' not in edge.sources
                            assert correct_word is None
                            correct_word = edge.value
                        if len(edge.sources) != 0:
                            fixed_edges[word] = edge
                    assert correct_word is not None
                    all_submissions = [(edge.value, edge.score, source, original_position)
                                       for edge in fixed_edges.values()
                                       for source, original_position in zip(edge.sources, edge.original_positions)
                                       ]
                    for word in fixed_edges:
                        submissions_sorted = sorted(
                            (calculate_wer(list(word), list(sub[0]))[1], ) + tuple(sub) for sub in all_submissions
                        )
                        prefeatures[id_][cost][position][word] = (submissions_sorted, word == correct_word)
        logging.debug('prefeatures extracted for {} rows'.format(len(self.rows)))
        return prefeatures

    def _count_eng_letters(self, text):
        x = findall(self.eng_letter, text)
        return len(x)

    def _get_word_features(self, word):
        if word is None:
            return [-100] * 4
        result = [
            len(word), self._count_eng_letters(word), len(word) - self._count_eng_letters(word), word == '?'
        ]
        return result

    def _get_word_to_word_features(self, word, hyp_word):
        if word is None or hyp_word is None:
            return [-100] * 3
        result = [
            calculate_wer(list(word), list(hyp_word))[1],
            len(word) - len(hyp_word),
            self._count_eng_letters(word) - self._count_eng_letters(hyp_word)
        ]
        return result

    @staticmethod
    def _extract_task_features(row):
        # mds_key = row['inputValues']['audio']
        return []

    def _extract_source_features(self):
        n_features = 8
        sources_stats = collections.defaultdict(lambda: np.zeros(n_features, dtype=np.int32))
        for row in self.rows:
            texts = []
            for cost, assignment in enumerate(sorted(row['raw_assesments'], key=lambda x: x['submitTs'])):
                cost += 1
                source_id = assignment['workerId']
                text = assignment['text']

                text = text.lower().replace('ё', 'е')
                texts.append(text)
                artificial = assignment['artificial']
                for i in range(cost, self.max_num_answers + 1):
                    sources_stats[(source_id, i)] += np.array([1,
                                                               text == '',
                                                               len(text.split()),
                                                               len(text),
                                                               artificial,  # number_of_speakers == 'many',
                                                               self._count_eng_letters(text),
                                                               self._count_eng_letters(text) != 0,
                                                               sum(1 for x in texts if x == text)
                                                               ])
        sources_stats[self.NO_SOURCE_ID] = np.full(fill_value=-1, shape=n_features)
        for i, value in sources_stats.items():
            n = value[0]
            value = np.concatenate((
                np.array([n], dtype=np.float),
                value[1:] / n
            ))
            sources_stats[i] = list(value)
        for i in range(1, self.max_num_answers + 1):
            sources_stats[(self.NO_SOURCE_ID, i)] = [-1] * n_features
        logging.debug('source features extracted for {} rows'.format(len(self.rows)))
        return dict(sources_stats)

    def _count_eng_words(self, text):
        return sum(1 for word in text.split() if self._count_eng_letters(word) != 0)

    def _extract_task_and_source_features(self):
        stats = dict()
        for row in self.rows:
            mds_key = row['inputValues']['audio']
            assignments = sorted(row['raw_assesments'], key=lambda x: x["submitTs"])
            for cost in range(1, self.max_num_answers + 1):
                texts = []
                for assignment in assignments[:cost]:
                    text = assignment['text']
                    text = text.lower().replace('ё', 'е')
                    texts.append(text)
                for assignment, text in zip(assignments[:cost], texts):
                    source_id = assignment['workerId']
                    artificial = assignment['artificial']
                    stats[(mds_key, source_id, cost)] = [
                        len(text.split()),
                        len(text),
                        artificial,  # number_of_speakers == "many",
                        self._count_eng_letters(text),
                        self._count_eng_words(text),
                        sum(1 for x in texts if x == text)
                    ]
            for i in range(1, self.max_num_answers + 1):
                stats[(mds_key, self.NO_SOURCE_ID, i)] = [
                    -1,
                    -1,
                    -1,
                    -1,
                    -1,
                    -1
                ]
        logging.debug('task and source features extracted for {} rows'.format(len(self.rows)))
        return stats

    def _extract_task_and_source_word_features(self):
        n_features = 5
        stats = dict()
        for row in self.rows:
            mds_key = row['inputValues']['audio']
            for assignment in row['raw_assesments']:
                source_id = assignment['workerId']
                text = assignment['text']
                text = text.lower().replace('ё', 'е')
                # submitTs = assignment['submitTs']
                text = text.split()
                text_len = len(text)
                for pos, word in enumerate(text):
                    stats[(mds_key, source_id, pos)] = [
                        pos,
                        len(text[pos - 1]) if pos > 0 else -1,
                        len(text[pos + 1]) if pos + 1 < text_len else -1,
                        self._count_eng_letters(text[pos - 1]) if pos > 0 else -1,
                        self._count_eng_letters(text[pos + 1]) if pos + 1 < text_len else -1
                    ]
                stats[(mds_key, source_id, None)] = [-1] * n_features
            stats[(mds_key, self.NO_SOURCE_ID, None)] = [-1] * n_features
        logging.debug('task and source word features extracted for {} rows'.format(len(self.rows)))
        return stats

    def extract_features(self):
        task_features = dict((row['inputValues']['audio'], self._extract_task_features(row)) for row in self.rows)
        source_features = self._extract_source_features()
        task_and_source_features = self._extract_task_and_source_features()
        task_and_source_word_features = self._extract_task_and_source_word_features()

        features = self._extract_prefeatures()
        for task_id, item0 in features.items():
            for cost, item1 in item0.items():
                for position, item2 in item1.items():
                    for word, item3 in item2.items():
                        prefeature, y = item3
                        prefeature += [(-1, None, None, self.NO_SOURCE_ID, None) for _ in
                                       range(self.max_num_answers - len(prefeature))]
                        if len(prefeature) != self.max_num_answers:
                            print(prefeature)
                            assert False
                        x = [position] + task_features[task_id] + self._get_word_features(word)

                        for edit_distance, hyp_word, score, source_id, original_position in prefeature:
                            x += self._get_word_features(hyp_word)
                            x += self._get_word_to_word_features(word, hyp_word)
                            x += source_features[(source_id, cost)]
                            x += task_and_source_features[(task_id, source_id, cost)]
                            x += task_and_source_word_features[(task_id, source_id, original_position)]
                        features[task_id][cost][position][word] = (x, y)
        logging.debug('all features extracted for {} rows'.format(len(self.rows)))
        return features


def main():
    setup_logger()
    context = parse_cmd_args()
    process(context)


if __name__ == '__main__':
    main()
