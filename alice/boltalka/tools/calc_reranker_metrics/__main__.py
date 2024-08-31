import numpy as np
import codecs, argparse, os
from collections import Counter, OrderedDict, defaultdict
import json
import yt.wrapper as yt
import sys
from alice.boltalka.tools.calc_reranker_metrics.utils import make_report
yt.config.set_proxy("hahn.yt.yandex.net")


# example: "result=0,not_male=1,not_rude=1,not_you=1,target_ass=,target_eng=,target_inf=0.120601326227"
def parse_targets(targets_str):
    targets = OrderedDict((name, float(value) if value != '' else None) for name, value in (pair.split('=') for pair in targets_str.split(',')))
    renamed_targets = OrderedDict()
    for name, value in targets.items():
        if name.startswith('not_'):
            renamed_targets[name.replace('not_', '')] = 1. - value if value is not None else None
        else:
            renamed_targets[name] = value
    return renamed_targets


def make_places(low, high=None):
    if high is None:
        high = low
    return np.stack([low, high], axis=1)


def get_places(scores):
    scores = np.sort(scores)[::-1]
    places_low = [0] * len(scores)
    places_high = [0] * len(scores)
    stride_begin = 0
    for i in range(1, len(places_low)):
        if scores[i] != scores[stride_begin]:
            for j in range(stride_begin, i):
                places_low[j] = (stride_begin + 1)
                places_high[j] = i
            stride_begin = i
    for j in range(stride_begin, len(places_low)):
        places_low[j] = (stride_begin + 1)
        places_high[j] = len(places_low)
    return make_places(places_low, places_high)


def average_by_places(func, places):
    return np.mean(func(np.arange(places[0], places[1] + 1)))


def calc_dcg(places, scores):
    s = 0
    for i in range(len(places)):
        s += average_by_places(lambda x: (np.exp2(scores[i]) - 1) / np.log2(x + 1), places[i])
    return s


def calc_ndcg(places, scores):
    dcg = calc_dcg(places, scores)
    idcg = calc_dcg(make_places(np.arange(len(scores)) + 1), -np.sort(-scores))
    if idcg == 0.:
        assert dcg == 0.
        return 1.
    return dcg / idcg


def calc_metrics(targets, rerank_by, top_sizes):
    metrics = {}
    for target_name in targets:
        target = np.array(targets[target_name])
        missing_values = target == None
        if missing_values.all():
            continue
        places = get_places(rerank_by[~missing_values])
        order = np.argsort(-rerank_by[~missing_values])
        ordered_target = target[~missing_values][order]
        for k in top_sizes:
            real_k = min(k, len(ordered_target))
            proba = (np.minimum(places[:, 1] + 1, k + 1) - np.minimum(places[:, 0], k + 1)) / (places[:, 1] - places[:, 0] + 1.0)
            if target_name == "result":
                metrics['%bad@{}'.format(k)] = ((ordered_target == 0) * proba).sum() / float(real_k)
                metrics['%neutral@{}'.format(k)] = ((ordered_target == 1) * proba).sum() / float(real_k)
                metrics['%good@{}'.format(k)] = ((ordered_target == 2) * proba).sum() / float(real_k)
                metrics['score@{}'.format(k)] = (ordered_target * proba).sum() / float(real_k)
            else:
                metrics['%{}@{}'.format(target_name, k)] = (ordered_target * proba).sum() / float(real_k)
        metrics['{}_ndcg'.format(target_name)] = calc_ndcg(places, ordered_target)
    return metrics


def drop_repeats(reply_hashes, targets, rerank_by):
    top_replies = {}
    for i in range(len(reply_hashes)):
        if reply_hashes[i] not in top_replies or rerank_by[i] > rerank_by[top_replies[reply_hashes[i]]]:
            top_replies[reply_hashes[i]] = i
    indices_to_delete = {i for i in range(len(reply_hashes)) if top_replies[reply_hashes[i]] != i}
    rerank_by = [v for i, v in enumerate(rerank_by) if i not in indices_to_delete]
    for name in targets:
        targets[name] = [v for i, v in enumerate(targets[name]) if i not in indices_to_delete]
    return targets, np.array(rerank_by)


class Reducer(object):
    def __init__(self, top_sizes, rerank_by_targets, no_rerank, inverse, unique_top):
        self.top_sizes = top_sizes
        self.rerank_by_targets = rerank_by_targets
        self.no_rerank = no_rerank
        self.inverse = inverse
        self.unique_top = unique_top

    def __call__(self, key, rows):
        targets = defaultdict(list)
        reply_hashes = []
        scores = []
        for row in rows:
            parts = row['value'].split('\t')
            scores.append(float(parts[-1]))
            targets_str, reply_hash = parts[1].split(';')
            target_dct = parse_targets(targets_str)
            reply_hashes.append(reply_hash)
            for k in target_dct:
                targets[k].append(target_dct[k])
        rerank_by = np.array(scores)
        if self.no_rerank:
            rerank_by = np.arange(len(scores) - 1, -1, -1)
        if self.rerank_by_targets:
            if any(np.equal(targets[t], None).all() for t in self.rerank_by_targets):
                return
            mask = np.prod(np.vstack([~np.equal(targets[t], None) for t in self.rerank_by_targets]).astype(int), axis=0) == 1
            for k in targets:
                targets[k] = [t for m, t in zip(mask, targets[k]) if m]
            rerank_by = np.argsort(np.lexsort([targets[t] for t in reversed(self.rerank_by_targets)]))
        if self.inverse:
            rerank_by = -rerank_by
        if self.unique_top:
            targets, rerank_by = drop_repeats(reply_hashes, targets, rerank_by)
        yield calc_metrics(targets, rerank_by, self.top_sizes)


def aggregate_metrics(table):
    metrics = Counter()
    freqs = Counter()
    for row in yt.read_table(table):
        metrics.update(row)
        freqs.update(row.keys())
    for name in metrics:
        metrics[name] /= float(freqs[name])
        if name.startswith('%'):
            metrics[name] *= 100
    return metrics


def get_logloss_on_test(training_log_json):
    with codecs.open(training_log_json, 'r', 'utf-8') as f:
        data = json.load(f)
    test_loss = [x['test'][0] for x in data['iterations']]
    iteration = np.argmin(test_loss)
    return test_loss[iteration], iteration


def main(args):
    top_sizes = map(int, args.top.split(','))
    rerank_by_targets = args.rerank_by_targets.split(',') if args.rerank_by_targets else []
    first_row_target_names = parse_targets(next(yt.read_table(args.src))['value'].split('\t')[1].split(';')[0]).keys()

    with yt.TempTable(os.path.dirname(args.src)) as tmp:
        if not yt.get(args.src + '/@sorted') or yt.get(args.src + '/@sorted_by')[0] != args.reduce_by:
            yt.run_sort(args.src, tmp, sort_by=args.reduce_by)
            args.src = tmp
        yt.run_reduce(Reducer(top_sizes, rerank_by_targets, args.no_rerank, args.inverse, args.unique_top), args.src, tmp, reduce_by=args.reduce_by, job_count=100)
        metrics = aggregate_metrics(tmp)

    metric_names = []
    for top_size in top_sizes:
        metric_names.extend([name+str(top_size) for name in ['%bad@', '%neutral@', '%good@', 'score@'] + ['%{}@'.format(name) for name in first_row_target_names if name != 'result']])
    metric_names.extend(['{}_ndcg'.format(name) for name in first_row_target_names])
    metric_names = [name for name in metric_names if name in metrics]

    if args.training_log:
        logloss_on_test, best_iter = get_logloss_on_test(args.training_log)
        metrics['logloss'] = logloss_on_test
        metrics['logloss_iter'] = best_iter
        metric_names = ['logloss', 'logloss_iter'] + metric_names

    if args.model_name:
        metrics['model'] = args.model_name
        metric_names.insert(0, 'model')

    metrics = OrderedDict((name, metrics[name]) for name in metric_names)

    print make_report([metrics])

    if args.output_json:
        with codecs.open(args.output_json, 'w', 'utf-8') as f:
            json.dump(metrics, f)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', required=True)
    parser.add_argument('--reduce-by', default='key')
    parser.add_argument('--model-name')
    parser.add_argument('--training-log')
    parser.add_argument('--rerank-by-targets', help='comma separated names')
    parser.add_argument('--no-rerank', action='store_true')
    parser.add_argument('--inverse', action='store_true')
    parser.add_argument('--unique-top', action='store_true')
    parser.add_argument('--top', type=str, default='1')
    parser.add_argument('--output-json')
    args = parser.parse_args()
    if args.no_rerank:
        assert args.rerank_by_target is None
    main(args)
