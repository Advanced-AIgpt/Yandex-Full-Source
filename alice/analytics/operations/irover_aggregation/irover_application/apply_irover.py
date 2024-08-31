# -*- coding: utf8 -*-

import argparse
import json
import logging
import sys
import numpy as np
import hashlib
from catboost import CatBoostClassifier
from collections import namedtuple, Counter, defaultdict


def reproducible_hash(s):
    # type: (str) -> str
    return hashlib.md5(s.encode('utf-8')).hexdigest()

def setup_logger():
    root_logger = logging.getLogger('')
    file_logger = logging.StreamHandler()
    file_logger.setFormatter(logging.Formatter('%(asctime)s %(levelname)-8s %(filename)s:%(lineno)d %(message)s'))
    root_logger.addHandler(file_logger)
    root_logger.setLevel(logging.DEBUG)


def parse_cmd_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('-ir', '--input_rows', dest='input_rows', help='json with assignments - sb urls')
    parser.add_argument('-if', '--input_features', dest='input_features', help='json with features')
    parser.add_argument('-clf', '--classifier', dest='classifier', help='catboost classifier model (cbm)',
                        required=True)
    parser.add_argument('-th', '--threshold', dest='threshold', help='threshold for prediction confidence',
                        required=True)
    parser.add_argument('--max_num_answers', metavar='MAX_NUM_ANSWERS',
                        help='Max number of assignments for one record from the pool', required=True)
    parser.add_argument('-a', '--aggregated', dest='aggregated', help='aggregated records', required=True)
    parser.add_argument('-c', '--confusing', dest='confusing', help='confusing tasks with assignments', required=True)
    parser.add_argument('-ch', '--check', dest='check', help='json to check the assignments', required=True)

    context = parser.parse_args(sys.argv[1:])

    return context


def write_result(name, result, context, is_json_list=False):
    logging.debug("{} results: {}".format(name, len(result)))
    with open(context.name, 'w', encoding="utf8") as out:
        if is_json_list:
            for obj in result:
                json.dump(obj, out, ensure_ascii=False)
                out.write('\n')
        else:
            json.dump(result, out, indent=4, ensure_ascii=False)


def md5(filename):
    hash_md5 = hashlib.md5()
    with open(filename, "rb") as f:
        for chunk in iter(lambda: f.read(4096), b""):
            hash_md5.update(chunk)
    return hash_md5.hexdigest()


def process(context):
    rows = json.loads(open(context.input_rows).read())
    features = json.loads(open(context.input_features).read())
    clf = CatBoostClassifier()
    clf.load_model(context.classifier, format='cbm')
    processor = Processor(rows, features, clf, context)
    processor.aggregate_irover()
    processor.fill_aggregated()
    write_result('aggregated', processor.aggregated, context)
    write_result('confusing', processor.confusing, context)
    write_result('check', processor.check, context)


AggregationResult = namedtuple('AggregationResult', 'text confidence cost')


class Processor:
    def __init__(self, rows, features, clf, context):
        self.rows = rows
        self.features = features
        self.clf = clf
        self.context = context
        self.MAX_NUM_ANSWERS = int(context.max_num_answers)
        self.threshold = float(context.threshold)

        self.aggregated = []
        self.confusing = []
        self.check = []
        self.results = {}

        self.completion = defaultdict(lambda: {'correct': 0.0, 'total': 0.0})   # ключи - assignmentId

    def _preaggregate_with_clf(self):
        matching = dict()
        x = list()
        for task_id, item0 in self.features.items():
            for cost, item1 in item0.items():
                cost = int(cost)
                for position, item2 in item1.items():
                    position = int(position)
                    for word, item3 in item2.items():
                        x, y = item3
                        x.append(np.array(x))
                        matching[(task_id, cost, position, word)] = len(x) - 1
        probabilities = self.clf.predict_proba(x)[:, 1]
        results = dict()
        for task_id, item0 in self.features.items():
            results[task_id] = dict()
            for cost, item1 in sorted(item0.items()):
                cost = int(cost)
                results[task_id][cost] = list()
                for position, item2 in sorted(item1.items(), key=lambda item: int(item[0])):
                    position = int(position)
                    words = list(item2.keys())
                    probs = np.array([probabilities[matching[(task_id, cost, position, word)]] for word in words])
                    pos = int(np.argmax(probs))
                    score = probs[pos]
                    word = words[pos]
                    results[task_id][cost].append((word, score))
        return results

    def aggregate_irover(self, min_cost=3):
        clf_results = self._preaggregate_with_clf()
        for task_id, item0 in clf_results.items():
            cost, item1 = max(item0.items())
            assert not (cost < min_cost or cost > self.MAX_NUM_ANSWERS), "Cost out of bounds"
            text = " ".join(value for value, score in item1 if value != "")
            score = sum(score for value, score in item1) / len(item1)
            self.results[task_id] = AggregationResult(text, score, cost)
        return self.results

    @staticmethod
    def extract_full_text(raw_assesments):
        texts = []
        for x in raw_assesments:
            text = x.get("annotation")
            if text is None:
                text = ""
            text = text.lower().replace(u'ё', u'е')
            texts.append(text)
        texts = Counter(texts)
        full_text, voices_full_text = max(texts.items(),
                                          key=lambda y: (
                                              y[1],
                                              y[0] != "",
                                              -y[0].count('?'),
                                              reproducible_hash(y[0])  # for reproducible behavior
                                          ))
        return full_text, voices_full_text

    def fill_aggregated(self):
        md5_clf_hash = md5(self.context.classifier)

        for row in self.rows:
            key = row['inputValues']['audio']
            agg_result = self.results.get(key)
            assert agg_result is not None, "'{key}' not in self.results".format(key=key)
            raw_assesments = row['raw_assesments']
            full_text, voices_full_text = self.extract_full_text(raw_assesments)
            url = row['inputValues']['audio']
            if agg_result.confidence >= self.threshold and agg_result.text != '':
                # add to aggregated (good)
                record = {
                    'query': agg_result.text,
                    'score_query': agg_result.confidence,
                    'full_text': full_text if voices_full_text >= 2 else None,
                    'voices_full_text': voices_full_text,
                    'url': url,
                    'mds_key': row['inputValues'].get('mds_key'),
                    'raw_assesments': raw_assesments,
                    'model_md5': md5_clf_hash
                }

                self.aggregated.append(record)

            else:
                # заполняем confusing
                if agg_result.text == "" and agg_result.confidence > self.threshold:
                    overlap = len(raw_assesments)
                else:
                    overlap = min(len(raw_assesments) + 1, self.MAX_NUM_ANSWERS)

                record = {
                    'id': row['taskId'],
                    'overlap': overlap,
                    'query': agg_result.text,
                    'score_query': agg_result.confidence,
                    'full_text': full_text if voices_full_text >= 2 else None,
                    'voices_full_text': voices_full_text,
                    'url': url,
                    'mds_key': row['inputValues'].get('mds_key'),
                    'raw_assesments': raw_assesments,
                    'model_md5': md5_clf_hash
                }

                self.confusing.append(record)

            # заполняем completion
            self._update_completions(row, agg_result)

        # заполняем check
        self._update_check()

    def _update_completions(self, row, agg_result):
        for answer in row['raw_assesments']:
            assignment_id = answer['assignmentId']
            self.completion[assignment_id]['total'] += 1
            if agg_result.confidence > self.threshold and answer['text'] == agg_result.text:
                self.completion[assignment_id]['correct'] += 1

    def _update_check(self):
        pool_id = self.rows[0]['poolId']
        for assign_id, counters in self.completion.items():
            try:
                if counters['total'] < 8 or (counters['correct'] / counters['total']) >= 0.25:
                    status = {"comment": "Thank you", "value": "APPROVE_SUBMITTED"}
                else:
                    status = {"comment": "Too many incorrect tasks", "value": "REJECT_SUBMITTED"}
                self.check.append({'assignmentId': assign_id, 'status': status, 'poolId': pool_id})
            except Exception as e:
                logging.exception('assignment %s problem: %s', assign_id, e)


def main():
    setup_logger()
    context = parse_cmd_args()
    process(context)


if __name__ == '__main__':
    main()
