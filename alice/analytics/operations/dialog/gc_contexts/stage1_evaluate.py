# -*-coding: utf8 -*-
import datetime
from collections import defaultdict
from itertools import chain

from utils.nirvana.op_caller import call_as_operation


SCORE = {
    "bad": 0.0,
    "neutral": 1.0,
    "good": 2.0,
}


def get_ds_stats(ds_result):
    scores = {}
    for obj in ds_result:
        source = obj['inputValues']['source']
        if source not in scores:
            scores[source] = {'bad': 0, 'neutral': 0, 'good': 0}
        scores[source][obj['outputValues']['result']] += 1
    return scores


def print_stats(title, stats, metrics):
    vote_sum = sum(stats.itervalues())
    total_score = 0
    for label, votes in stats.iteritems():
        vote_fraction = float(votes) / vote_sum
        total_score += SCORE[label] * vote_fraction
        metrics[title + '_' + label] = votes * 100.0 / sum(stats.itervalues())
        #print '%s (score=%.1f): %.1f%%' % (label, scores[label], votes * 100.0 / sum(stats.itervalues()))
    if title == 'ds':
        metrics[title + '_score'] = total_score
    else:
        metrics[title + '_total'] = total_score
    return metrics


def process_source(scr_tasks, ds_stats, source, fielddate):
    hard_stats = {}
    soft_stats = {}

    for item in scr_tasks:
        result = item['outputValues']['result']
        majority_winner = max(result.iteritems(), key=lambda r: r[1])[0]
        hard_stats[majority_winner] = hard_stats.get(majority_winner, 0) + 1
        total_votes = sum(result.itervalues())
        for label, label_votes in result.iteritems():
            vote_fraction = float(label_votes) / total_votes
            soft_stats[label] = soft_stats.get(label, 0) + vote_fraction

    metrics = {}
    print_stats('mv', hard_stats, metrics)
    print_stats('wmv', soft_stats, metrics)
    print_stats('ds', ds_stats, metrics)

    if fielddate is None:
        metrics["fielddate"] = datetime.datetime.now().strftime("%Y-%m-%d")
    else:
        metrics["fielddate"] = fielddate

    metrics["modelname"] = source
    return metrics


def main(raw_assigns, aggr_assigns, fielddate=None):
    """
    Вычисление метрик из оценок толокеров
    :param list[list[dict]] raw_assigns: Список пулов из ответов толокеров
    :param list[dict] aggr_assigns: Список сагрегированных ответов
    :param str|None fielddate: Дата, которая будет подставляться в выходные данные для заливки на stat
    :return:
    """
    tasks = list(chain.from_iterable(raw_assigns))
    source_split = defaultdict(list)
    for tsk in tasks:
        source_split[tsk['inputValues']['source']].append(tsk)

    ds_stats = get_ds_stats(aggr_assigns)

    metrics = [process_source(scr_tasks, ds_stats[src], src, fielddate)
               for src, scr_tasks in source_split.iteritems()]

    return metrics


if __name__ == '__main__':
    call_as_operation(main, input_spec={
        'raw_assigns': {'link_name': 'raw_assigns', 'parser': 'json', 'as_list': True},
        'aggr_assigns': {'link_name': 'aggr_assigns', 'parser': 'json'},
    })
