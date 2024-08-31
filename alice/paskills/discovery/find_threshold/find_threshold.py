#!/usr/local/bin/python
# -*- coding: utf-8 -*-

import argparse
import sys
import six
import json
from io import open
from collections import Counter
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
from sklearn import metrics

def docs_by_thresholds(y_scores, doc_ids, thresholds):
    threshold_doc_counts = []
    diff_docs = set()
    docs_weight = 0
    for score, doc_id in reversed(sorted([(x[0],x[1]) for x in zip(y_scores, doc_ids)])):
        threshold_doc_counts.append((score, docs_weight))
        if doc_id not in diff_docs:
            docs_weight += 1
            diff_docs.add(doc_id)
    threshold_doc_counts = list(sorted(threshold_doc_counts))

    docs = []
    cur = 0
    docs.append(threshold_doc_counts[0][1])
    for t in thresholds:
        while threshold_doc_counts[cur][0] < t:
            cur += 1
        docs.append(threshold_doc_counts[cur][1])

    return np.array(docs)

class Responses:
    def __init__(self, path, total_queries, max_response_pos):
        self.doc_ids = []
        self.doc_pos = []
        self.y_scores = []
        self.y_true = []
        self.total_queries = total_queries
        with open(path) as f:
            for line in f:
                sp = line.rstrip().split('\t')
                weight = int(sp[3])
                query_id = int(sp[0])
                resp_pos = int(sp[4])
                for i in range(weight):
                    if resp_pos > max_response_pos:
                        continue
                    self.doc_ids.append(i*1000000 + query_id)
                    self.y_scores.append(float(sp[1]))
                    self.y_true.append(float(sp[2]))
                    self.doc_pos.append(resp_pos)

        self.precision, self.recall, self.thresholds = metrics.precision_recall_curve(self.y_true, self.y_scores)
        self.docs = docs_by_thresholds(self.y_scores, self.doc_ids, self.thresholds)
        self.docs = self.docs/float(self.total_queries)

    def roc_auc_plot(self, ax=None):
        if ax is None:
            ax = plt.gca()
        fpr, tpr, thresholds = metrics.roc_curve(self.y_true, self.y_scores, pos_label=1.)
        ax.set_xlabel('fpr')
        ax.set_ylabel('tpr')
        ax.set_ylim([0.0, 1.05])
        ax.set_xlim([0.0, 1.0])
        ax.plot(fpr, tpr)
        ax.set_title('ROC-AUC curve: AUC={0:0.2f}'.format(metrics.roc_auc_score(self.y_true, self.y_scores)))

    def precision_recall_plot(self, ax=None):
        if ax is None:
            ax = plt.gca()
        precision, recall, thresholds = metrics.precision_recall_curve(self.y_true, self.y_scores)
        average_precision = metrics.average_precision_score(self.y_true, self.y_scores)
        ax.step(recall, precision, color='b', alpha=0.2,
                 where='post')
        ax.fill_between(recall, precision, step='post', alpha=0.2,
                         color='b')

        ax.set_xlabel('Recall')
        ax.set_ylabel('Precision')
        ax.set_ylim([0.0, 1.05])
        ax.set_xlim([0.0, 1.0])
        ax.set_title('Precision-Recall curve: AP={0:0.2f}'.format(
                  average_precision))

    def precision_docs_plot(self, ax=None, xlim=None):
        if ax is None:
            ax = plt.gca()
        ax.plot(self.docs, self.precision)
        ax.set_xlabel('Sth found')
        ax.set_ylabel('Precision')

        if xlim is not None:
            ax.set_xlim([0.0, xlim])
        ax.set_title('Precision-Docs curve')

    def plot_all(self, xlim=None):
        f, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(15,15))
        f.suptitle('Postive labels: %d, Total labels: %d, Queries: %d' % (sum(self.y_true), len(self.y_true), self.total_queries))
        self.roc_auc_plot(ax1)
        self.precision_recall_plot(ax2)
        self.precision_docs_plot(ax3)
        if xlim is not None:
            self.precision_docs_plot(ax4, xlim)

    def find_threshold(self, precision=None, found_percent=None, threshold=None):
        if precision is None and found_percent is None and threshold is None:
            raise ValueError("add arguments")
        for p, r, d, t in zip(self.precision, self.recall, self.docs, self.thresholds):
            if found_percent is not None and d < found_percent:
                return(p, r, d, t)
            if threshold is not None and t >= threshold:
                return(p, r, d, t)
            if precision is not None and p > precision:
                return(p, r, d, t)


def load_relevance(relevance_dict, stream):
    for line in stream:
        d = json.loads(line)
        relevance_dict[(d['query'].strip().lower(), d['skill_id'])] = 1 if d['answer'] == 'YES' or d['golden'] == 'YES' else 0


def load_stable(relevance_dict, stream, add_prefix):
    for line in stream:
        d = json.loads(line)
        if d['channel'] != 'aliceSkill':
            continue
        if d['isBanned']:
            continue
        if d['hideInStore']:
            continue
        if d['deletedAt']:
            continue
        if not d['onAir']:
            continue

        for phrase in d['activationPhrases']:
            if not add_prefix:
                relevance_dict[(phrase.strip().lower(), d['id'])] = 1
                continue

            for prefix in [u"открой навык ", u"открой диалог ", u"запусти навык ", u"запусти диалог ", u"активируй навык ", u"включи навык ", u"вызови навык ", u"открой чат ", u"запусти чат с "]:
                relevance_dict[((prefix + phrase).strip().lower(), d['id'])] = 1
            if d['category'] not in ["games_trivia_accessories", "kids"]:
                continue
            for prefix in [u"давай поиграем в ", u"давай сыграем в "]:
                relevance_dict[((prefix + phrase).strip().lower(), d['id'])] = 1


def get_resp_dict(resp):
    if isinstance(resp, dict):
        return resp
    return {'url': resp[0], 'relevance': resp[1]}


def scores(stream, relevance_dict, top, counters):
    for i, line in enumerate(stream):
        d = json.loads(line)
        if 'response' not in d:
            continue
        query = d['query']['query'].strip().lower()
        weight = d['query']['weight'] if 'weight' in d['query'] else 1
        response = [get_resp_dict(x) for x in d['response']]
        counters['queries'] += 1
        counters['queries_weighted'] += weight
        if len(response) == 0:
            continue
        for pos, resp in enumerate(response[:top]):
            yield (i, resp['relevance'], relevance_dict.get((query, resp['url']), 0), weight, pos)

def main():
    parser = argparse.ArgumentParser(prog='Score saas results')
    parser.add_argument('--relevance', required=True)
    parser.add_argument('--skills')
    parser.add_argument('--counters-out')
    parser.add_argument('--thresholds-out', required=True)
    parser.add_argument('--plot-out-file')
    parser.add_argument('--found-threshold-out')
    parser.add_argument('--top', type=int, default=10)
    parser.add_argument('--fourth-xlim', type=float, default=0.05)
    parser.add_argument('--precision', type=float)
    parser.add_argument('--found-percent', type=float)
    parser.add_argument('--threshold', type=float)

    args = parser.parse_args()

    relevance_dict = dict()
    if args.skills:
        with open(args.skills) as f:
            load_stable(relevance_dict, f, False)
        with open(args.skills) as f:
            load_stable(relevance_dict, f, True)

    with open(args.relevance) as f:
        load_relevance(relevance_dict, f)

    counters = Counter()
    with open (args.thresholds_out, 'wb') as f:
        for query_id, y_score, y_true, weight, pos in scores(sys.stdin, relevance_dict, args.top, counters):
            six.print_(query_id, y_score, y_true, weight, pos, sep=u'\t', file=f)

    if args.counters_out is not None:
        with open(args.counters_out, 'wb') as f:
            json.dump(counters, f, sort_keys=True, indent=4, separators=(u',', u': '))

    resps = Responses(args.thresholds_out, counters['queries_weighted'], args.top)

    if args.precision is not None or args.found_percent is not None or args.threshold is not None:
        p, r, d, t = resps.find_threshold(args.precision, args.found_percent, args.threshold)
        with open(args.found_threshold_out, 'wb') as f:
            json.dump({u'precision': p, u'recall': r, u'found_percent': d, u'threshold': int(t)}, f, sort_keys=True, indent=4, separators=(u',', u': '))

    if args.plot_out_file is not None:
        plt.ioff()
        resps.plot_all(xlim=args.fourth_xlim)
        plt.savefig(args.plot_out_file)

if __name__ == "__main__":
    main()
