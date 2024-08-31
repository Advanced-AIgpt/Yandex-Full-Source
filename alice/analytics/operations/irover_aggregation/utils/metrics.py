# -*-coding: utf8 -*-
from __future__ import absolute_import
from __future__ import print_function

import hashlib
from collections import Counter

# from typing import Tuple, Any
from .word_transition_network import *

AggregationResult = collections.namedtuple('AggregationResult', 'text confidence cost')


def reproducible_hash(s):
    # type: (str) -> str
    return hashlib.md5(s.encode('utf-8')).hexdigest()


def aggregate_prod(raw_data):
    """
       aggregation from prod
    """
    cost = 2
    while cost < 5:
        cost += 1
        answers = [(x["text"], x["speech"]) for x in raw_data[:cost]]
        answers = Counter(answers)
        if answers.most_common(1)[0][1] >= 3:
            break

    texts = Counter()
    speechs = Counter()
    for text, speech in [(x["text"], x["speech"]) for x in raw_data[:cost]]:
        if speech != "BAD" and text:
            text = text.lower().replace(u'ё', u'е')
        else:
            text = ""
        speechs.update([speech])
        texts.update([text])

    text, text_rate = max(texts.items(),
                          key=lambda y: (
                              y[1],
                              y[0] != "",
                              -y[0].count('?'),
                              reproducible_hash(y[0])  # for reproducible behavior
                          ))
    if text_rate >= 2:  # and text != "":
        conf = text_rate * 1.0 / sum(texts.values())
    else:
        text = None
        conf = 0
    return AggregationResult(text, conf, cost)


def evaluate_metrics_for_algorithm(data, field, algorithm, threshold=.0, cluster_refernces=None, print_=True):
    # type: (Any, str, Any, float, Any, bool) -> Tuple[float, float, float]
    """
    Apply given aggregation algorithm and calculates metrics
    :param data: list of dicts with "text" and field keys
    :param field: field name to take from data as hyps source for aggregation
    :param algorithm: function to be applied
    :param threshold: ignore results with confidence lower then treshhold
    :param cluster_refernces: ClusterReference or None
    :param print_: print readable results of evaluation
    :return: tuple(float, float, float), WER computed for set, recall, mean overlap
    """
    recall = 0.0
    wer_words = 0.0
    wer_errors = 0.0
    cost = 0.0
    total_items = 0
    for row in data:
        if row["mark"] != "TEST":
            continue
        total_items += 1
        hyp = algorithm(sorted(row[field], key=lambda x: x["submit_ts"]))
        cost += hyp.cost
        if hyp.confidence < threshold:
            hyp = None
        else:
            hyp = hyp.text
        ref = row["text"]
        wer_item = calculate_wer(ref, hyp, cluster_references=cluster_refernces)
        wer_words += wer_item[2]
        wer_errors += wer_item[1]
        recall += calculate_word_recall(ref, hyp, cluster_references=cluster_refernces)[0]

    wer = wer_errors / wer_words
    recall /= total_items
    cost /= total_items
    if print_:
        print("Recall: {:.4%}\nWER: {:.4%}\nMean overlap: {:.4}".format(
            recall, wer, cost
        ))
    return recall, wer, cost


def evaluate_metrics_from_dict(data, aggregation_result, threshold=.0,
                               cluster_references=None, print_=True):
    # (Any, Dict, Any, float, Any, bool) -> Tuple[float, float, float]
    """
    Take aggregation results from dict and calculates metrics
    :param data: list of dicts with "text" and "mds_key" keys
    :param aggregation_result: dict, mds_key -> aggregation hyp
    :param threshold: ignore results with confidence lower then treshhold
    :param cluster_references: ClusterReference or None
    :param print_: print readable results of evaluation
    :return: tuple(float, float, float), WER computed for set, recall, mean overlap
    """
    recall = 0.0
    wer_words = 0.0
    wer_errors = 0.0
    cost = 0.0
    total_items = 0
    for row in data:
        if row["mark"] != "TEST":
            continue
        total_items += 1
        hyp = aggregation_result[row["mds_key"]]
        cost += hyp.cost
        if hyp.confidence < threshold:
            hyp = None
        else:
            hyp = hyp.text
        ref = row["text"]
        wer_item = calculate_wer(ref, hyp, cluster_references=cluster_references)
        wer_words += wer_item[2]
        wer_errors += wer_item[1]
        recall += calculate_word_recall(ref, hyp, cluster_references=cluster_references)[0]

    wer = wer_errors / wer_words
    recall /= total_items
    cost /= total_items
    if print_:
        print("Recall: {:.4%}\nWER: {:.4%}\nMean overlap: {:.4}".format(
            recall, wer, cost
        ))
    return recall, wer, cost


def evaluate_metrics_from_texts(texts, cluster_refernces=None, print_=True):
    # (List[Tuple[str, str]], Any, bool) -> Tuple[float, float]
    """
    calculates metrics from ref and hyp texts
    :param texts: list of tuples (reference, hypothesis)
    :param cluster_refernces: ClusterReference or None
    :param print_: print readable results of evaluation
    :return: tuple(float, float), WER computed for set, recall
    """
    recall = 0.0
    wer_words = 0.0
    wer_errors = 0.0
    total_items = 0
    for ref, hyp in texts:
        total_items += 1
        wer_item = calculate_wer(ref, hyp, cluster_references=cluster_refernces)
        wer_words += wer_item[2]
        wer_errors += wer_item[1]
        recall += calculate_word_recall(ref, hyp, cluster_references=cluster_refernces)[0]

    wer = wer_errors / wer_words
    recall /= total_items
    if print_:
        print("Recall: {:.4%}\nWER: {:.4%}".format(
            recall, wer
        ))
    return recall, wer
