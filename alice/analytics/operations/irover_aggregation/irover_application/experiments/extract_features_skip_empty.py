# -*- coding: utf8 -*-

import argparse
import json
import logging
import sys
from collections import defaultdict, Counter
from re import findall, compile
from tqdm import tqdm_notebook

try:
    from word_transition_network import *
except ModuleNotFoundError:
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
    parser.add_argument('--platform', metavar='PLATFORM', help='name of the platform, e.g. \"toloka\"/\"yang\"',
                        required=True)
    parser.add_argument('--max_num_answers', metavar='MAX_NUM_ANSWERS', help='Max number of assignments for one record '
                                                                             'from the pool',
                        required=True)
    parser.add_argument('--for_fitting', help='Should extract features for all costs for fitting? default: False',
                        action="store_true")
    parser.add_argument('--rows', dest='rows', help='rows for evaluation')
    parser.add_argument('--features', dest='features', help='features of rows to evaluate')
    context = parser.parse_args(sys.argv[1:])

    return context


def write_result(name, result, context, is_json_list=False):
    logging.debug("{} results: {}".format(name, len(result)))
    with open(getattr(context, name), 'w') as out:
        if is_json_list:
            for obj in result:
                json.dump(obj, out, ensure_ascii=False)
                out.write('\n')
        else:
            json.dump(result, out, indent=4, ensure_ascii=False)


def process(context):
    assignments = json.loads(open(context.input, encoding="utf8").read())
    logging.debug('load {} assignments'.format(len(assignments)))
    processor = Processor(assignments, context)
    features = processor.extract_features()
    rows = processor.get_rows()
    write_result('rows', rows, context)
    write_result('features', features, context)


class Processor:
    def __init__(self, assignments, context):
        self.assignments = assignments
        self.context = context
        self.MAX_NUM_ANSWERS = int(context.max_num_answers)
        self.for_fitting = context.for_fitting
        self.NO_SOURCE_ID = 'NO WORKER'
        self.eng_letter = compile("[a-zA-Z]")

        self.rows = []
        self.features = {}

        # self.prepare_data()

    def prepare_data(self):
        task_answers = defaultdict(lambda: {'raw_assesments': []})
        for answer in self.assignments:
            input_values = answer['inputValues']
            key = frozenset(input_values.values())
            value = {
                'platform': self.context.platform,
                'submit_ts': answer['submitTs'],
                'worker_id': answer['workerId'],
                'assignmentId': answer['assignmentId']
            }
            value.update(answer['outputValues'].copy())
            # временно ? удаляем из строки
            # value['text'] = value['result'].replace(' ?', '')
            # value['text'] = value['text'].replace('? ', '')
            # value['text'] = value['text'].replace('?', '')
            value['text'] = value['result']

            del value['result']
            value['number_of_speakers'] = self._extract_number_of_speakers(value['speech'])

            task_answers[key]['inputValues'] = input_values
            task_answers[key]['taskId'] = answer['taskId']
            task_answers[key]['raw_assesments'].append(value)
            task_answers[key]['poolId'] = answer['poolId']

        for key, value in task_answers.items():
            self.rows.append({
                'inputValues': value['inputValues'],
                'raw_assesments': value['raw_assesments'],
                'poolId': value['poolId'],
                'taskId': value['taskId']
            })
        logging.debug('prepared {} rows'.format(len(self.rows)))

    def get_rows(self):
        return self.rows

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
        for row in tqdm_notebook(self.rows):
            toloka_answers = sorted(row['raw_assesments'], key=lambda x: x['submit_ts'])
            id_ = row['inputValues']['url']
            if self.for_fitting:
                cost_range = range(1, self.MAX_NUM_ANSWERS + 1)
            else:
                cost_range = [self.MAX_NUM_ANSWERS]
            for cost in cost_range:
                hyps = []
                for text, speech, worker_id in [(x['text'], x['speech'], x['worker_id']) for x in
                                                toloka_answers[:cost]]:
                    if speech != 'BAD' and text:
                        text = text.lower().replace('ё', 'е')
                    else:
                        text = ''
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
                    # пропускаем добавочную пустую строку между словами
                    if len(expanded_alignment) > 1 and position % 2 == 0:
                        continue
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
                            (calculate_wer(list(word), list(sub[0]))[1], *sub) for sub in all_submissions
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
        mds_key = row['inputValues']['url']
        return []

    def _extract_source_features(self):
        N_FEATURES = 8
        sources_stats = collections.defaultdict(lambda: np.zeros(N_FEATURES, dtype=np.int32))
        for row in self.rows:
            texts = []
            for cost, assignment in enumerate(sorted(row['raw_assesments'], key=lambda x: x['submit_ts'])):
                cost += 1
                source_id = assignment['worker_id']
                text = assignment['text']

                speech = assignment['speech']
                if speech != 'BAD' and text:
                    text = text.lower().replace('ё', 'е')
                else:
                    text = ''
                texts.append(text)
                number_of_speakers = assignment['number_of_speakers']
                for i in range(cost, self.MAX_NUM_ANSWERS + 1):
                    sources_stats[(source_id, i)] += np.array([1,
                                                               text == '',
                                                               len(text.split()),
                                                               len(text),
                                                               number_of_speakers == 'many',
                                                               self._count_eng_letters(text),
                                                               self._count_eng_letters(text) != 0,
                                                               sum(1 for x in texts if x == text)
                                                               ])
        sources_stats[self.NO_SOURCE_ID] = np.full(fill_value=-1, shape=N_FEATURES)
        for i, value in sources_stats.items():
            n = value[0]
            value = np.concatenate((
                np.array([n], dtype=np.float),
                value[1:] / n
            ))
            sources_stats[i] = list(value)
        for i in range(1, self.MAX_NUM_ANSWERS + 1):
            sources_stats[(self.NO_SOURCE_ID, i)] = [-1] * N_FEATURES
        logging.debug('source features extracted for {} rows'.format(len(self.rows)))
        return dict(sources_stats)

    def _count_eng_words(self, text):
        return sum(1 for word in text.split() if self._count_eng_letters(word) != 0)

    def _extract_task_and_source_features(self):
        stats = dict()
        for row in self.rows:
            mds_key = row['inputValues']['url']
            assignments = sorted(row['raw_assesments'], key=lambda x: x["submit_ts"])
            for cost in range(1, len(assignments) + 1):
                texts = []
                for assignment in assignments[:cost]:
                    text = assignment["text"]
                    speech = assignment["speech"]
                    if speech != "BAD" and text:
                        text = text.lower().replace('ё', 'е')
                    else:
                        text = ""
                    texts.append(text)
                for assignment, text in zip(assignments[:cost], texts):
                    source_id = assignment["worker_id"]
                    number_of_speakers = assignment["number_of_speakers"]
                    stats[(mds_key, source_id, cost)] = [
                        len(text.split()),
                        len(text),
                        number_of_speakers == "many",
                        self._count_eng_letters(text),
                        self._count_eng_words(text),
                        sum(1 for x in texts if x == text)
                    ]

            for i in range(1, self.MAX_NUM_ANSWERS + 1):
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
        N_FEATURES = 5
        stats = dict()
        for row in self.rows:
            mds_key = row['inputValues']['url']
            for assignment in row['raw_assesments']:
                source_id = assignment['worker_id']
                text = assignment['text']
                speech = assignment['speech']
                if speech != 'BAD' and text:
                    text = text.lower().replace('ё', 'е')
                else:
                    text = ''
                number_of_speakers = assignment['number_of_speakers']
                submit_ts = assignment['submit_ts']
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
                stats[(mds_key, source_id, None)] = [-1] * N_FEATURES
            stats[(mds_key, self.NO_SOURCE_ID, None)] = [-1] * N_FEATURES
        logging.debug('task and source word features extracted for {} rows'.format(len(self.rows)))
        return stats

    def extract_features(self):
        task_features = dict((row['inputValues']['url'], self._extract_task_features(row)) for row in self.rows)
        source_features = self._extract_source_features()
        task_and_source_features = self._extract_task_and_source_features()
        task_and_source_word_features = self._extract_task_and_source_word_features()

        print('extracting prefeatures...')
        features = self._extract_prefeatures()
        
        print('extracting final features...')
        for task_id, item0 in tqdm_notebook(features.items()):
            for cost, item1 in item0.items():
                for position, item2 in item1.items():
                    for word, item3 in item2.items():
                        prefeature, y = item3
                        prefeature += [(-1, None, None, self.NO_SOURCE_ID, None) for _ in
                                       range(self.MAX_NUM_ANSWERS - len(prefeature))]
                        if len(prefeature) != self.MAX_NUM_ANSWERS:
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

    @staticmethod
    def _extract_number_of_speakers(speech):
        if speech == 'BAD':
            return None
        if speech == 'OK_one_speaker':
            return 'one'
        if speech == 'OK_many_speakers':
            return 'many'
        raise ValueError('Unknown value of speech type')


def main():
    setup_logger()
    context = parse_cmd_args()
    process(context)


if __name__ == '__main__':
    main()
