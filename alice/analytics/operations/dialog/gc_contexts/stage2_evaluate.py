# -*-coding: utf8 -*-
import datetime
from collections import defaultdict, Counter

from utils.nirvana.op_caller import call_as_operation


def compute_metrics(results, source, fielddate=None):
    dim = ['male', 'you', 'rude']
    stats = Counter(tuple(x['outputValues'][d] for d in dim)
                    for x in results)
    metrics = {}

    for idx, d in enumerate(dim):
        metrics[d] = sum(stats[x]
                         for x in stats
                         if x[idx] == 'YES')

    metrics['total'] = sum(stats[x]
                           for x in stats
                           if "YES" in x)

    total = float(sum(stats.values()))
    for k, v in metrics.iteritems():
        metrics[k] = v / total

    if fielddate is None:
        metrics["fielddate"] = datetime.datetime.now().strftime("%Y-%m-%d")
    else:
        metrics["fielddate"] = fielddate

    metrics['source'] = source
    return metrics


def main(tasks, fielddate=None):
    """
    Вычисление метрик из оценок толокеров
    :param list[dict] tasks: Сагрегированные оценки толокеров
    :param str|None fielddate: Дата, которая будет подставляться в выходные данные для заливки на stat
    :return:
    """
    source_split = defaultdict(list)
    for tsk in tasks:
        source_split[tsk['inputValues']['source']].append(tsk)

    metrics = []
    for source in source_split.iterkeys():
        m = compute_metrics(source_split[source], source, fielddate)
        metrics.append(m)

    return metrics


if __name__ == '__main__':
    call_as_operation(main, input_spec={'tasks': {'parser': 'json'}})
