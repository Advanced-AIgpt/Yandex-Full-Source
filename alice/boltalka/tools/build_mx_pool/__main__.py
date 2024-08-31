# coding=utf-8
import argparse
import yt.wrapper as yt
from collections import Counter
import hashlib
yt.config.set_proxy('hahn.yt.yandex.net')

def sample_table_mapper(hash_modulo, train_remainder, is_test, row):
    is_train_sample = int(row['query_id'], 16) % hash_modulo < train_remainder
    if is_train_sample != is_test:
        yield row

_RESULT_MAP = {
    'good': 2,
    'neutral': 1,
    'bad': 0
}

ASSESSMENT_METRICS = ['result', 'not_male', 'not_rude', 'not_you']
def get_metric_names(row):
    return ASSESSMENT_METRICS + sorted(name for name in row.keys() if name.startswith('target_'))

def map_sample(row):
    features = []
    i = 0
    while 'factor_' + str(i) in row:
        features.append(str(row['factor_' + str(i)]))
        i += 1
    query = []
    i = 0
    while 'query_' + str(i) in row:
        query.append(row['query_' + str(i)])
        i += 1
    res = {
        'query_id': row['query_id'],
        'features': '\t'.join(features),
        'reply': row['reply'],
        'query': '\t'.join(query[::-1]),
        'table_id': int(row.get('table_id', '0')),
    }
    res.update({k: v for k, v in row.iteritems() if k.startswith('target_')})
    res['result'] = _RESULT_MAP[row['result']] if 'result' in row else None
    for metric in ['male', 'rude', 'you']:
        res['not_' + metric] = int(row[metric] == 'NO') if metric in row else None
    assert all(m in res for m in ASSESSMENT_METRICS)
    yield res

def get_major_target_id(x, y, accept_thresholds, deny_thresholds):
    for i in xrange(len(x)):
        if x[i] is None or y[i] is None:
            continue
        if x[i] - y[i] < deny_thresholds[i]:
            return None
        if x[i] - y[i] > accept_thresholds[i]:
            return i
    return None

def get_metrics_diff(x, y, accept_thresholds, deny_thresholds):
    num_targets = len(accept_thresholds)
    diff = []
    for i in xrange(len(x)):
        if x[i] is None or y[i] is None:
            diff.append(0)
        elif x[i] - y[i] < (deny_thresholds[i] if i < num_targets else 0.):
            diff.append(-1)
        elif x[i] - y[i] > (accept_thresholds[i] if i < num_targets else 0.):
            diff.append(1)
        else:
            diff.append(0)
    return diff

def get_pair_weight_by_num_pairs(targets, normalize_by_num_pairs, priority_weights, accept_thresholds, deny_thresholds):
    if normalize_by_num_pairs == 'none':
        return 1.0
    num_pairs = 0
    for x in targets:
        for y in targets:
            major_target_id = get_major_target_id(x, y, accept_thresholds, deny_thresholds)
            if major_target_id is not None:
                num_pairs += priority_weights[major_target_id]
    if num_pairs == 0:
        return 1.0
    if normalize_by_num_pairs == 'count':
        return 1.0 / num_pairs
    if normalize_by_num_pairs == 'sqrt':
        return 1.0 / num_pairs**0.5

    raise Exception('Unknown normalize by num pairs mode: ' + normalize_by_num_pairs)

@yt.with_context
class Reducer(object):
    def __init__(self, priority, priority_weights, accept_thresholds, deny_thresholds, normalize_by_num_pairs, use_priority_weights_for_pair_count, s2s_sample_weight):
        self.priority = priority
        self.priority_weights = priority_weights
        self.accept_thresholds = accept_thresholds
        self.deny_thresholds = deny_thresholds
        self.normalize_by_num_pairs = normalize_by_num_pairs
        self.use_priority_weights_for_pair_count = use_priority_weights_for_pair_count
        self.s2s_sample_weight = s2s_sample_weight

    def __call__(self, key, rows, context):
        targets = []
        metrics = []
        ids = []
        replies = []
        is_s2s = []
        query_id = key['query_id']
        for i, row in enumerate(rows):
            if i == 0:
                table_id = row['table_id']
                priority = self.priority[table_id]
                metric_names = get_metric_names(row)
                metric_names_ordered = priority + [m for m in metric_names if m not in priority]
                query = row['query']
            t = str(row['result']) if row['result'] is not None else '-1'
            u = ','.join('{}={}'.format(name, row[name] if row[name] is not None else '') for name in metric_names)
            w = str(i)
            targets.append(tuple(row.get(k) for k in priority))
            metrics.append(tuple(row.get(k) for k in metric_names_ordered))
            ids.append(context.row_index)
            replies.append(row['reply'])
            reply_hash = hashlib.md5(replies[-1]).hexdigest()
            u += ';' + reply_hash
            is_s2s.append(row.get('target_s2s', 0))
            yield yt.create_table_switch(0)
            yield {
                'key': query_id,
                'value': '\t'.join((t, u, w, row['features'])),
                'id': ids[-1],
                'table_id': table_id,
            }

        if not priority:
            return

        priority_weights = self.priority_weights[table_id]
        accept_thresholds = self.accept_thresholds[table_id]
        deny_thresholds = self.deny_thresholds[table_id]

        priority_weights_for_pair_count = priority_weights if self.use_priority_weights_for_pair_count else [1] * len(priority)
        pair_weight = get_pair_weight_by_num_pairs(targets, self.normalize_by_num_pairs, priority_weights_for_pair_count, accept_thresholds, deny_thresholds)

        pairs_cntr = Counter()
        implicit_pairs_cntr = {1: Counter(), -1: Counter(), 0: Counter()}

        for i in range(len(targets)):
            for j in range(len(targets)):
                major_target_id = get_major_target_id(targets[i], targets[j], accept_thresholds, deny_thresholds)
                if major_target_id is None:
                    continue
                s2s_coef = self.s2s_sample_weight ** (is_s2s[i] + is_s2s[j])
                weight = pair_weight * priority_weights[major_target_id] * s2s_coef
                yield yt.create_table_switch(1)
                yield {
                    'pos_id': ids[i],
                    'neg_id': ids[j],
                    'weight': weight,
                    'pos_reply': replies[i],
                    'neg_reply': replies[j],
                    'query': query,
                    'table_id': table_id,
                    'target_diff': ['\t'.join([priority[major_target_id], str(targets[i][major_target_id]), str(targets[j][major_target_id])]) for target_id in range(len(priority))],
                }
                pairs_cntr[major_target_id] += 1
                for metric_id, value in enumerate(get_metrics_diff(metrics[i], metrics[j], accept_thresholds, deny_thresholds)):
                    implicit_pairs_cntr[value][metric_id] += 1
        yield yt.create_table_switch(2)
        for metric_id in range(len(metric_names_ordered)):
            implicit_pairs_cntr[1][metric_id] -= pairs_cntr.get(metric_id, 0)
            metric_name = metric_names_ordered[metric_id] + '#' + str(metric_id) if metric_id < len(priority) else metric_names_ordered[metric_id]
            yield {'metric': metric_name,
                   'table_id': table_id,
                   'count': pairs_cntr.get(metric_id, 0),
                   'implicit_pos_count': implicit_pairs_cntr[1][metric_id],
                   'implicit_neg_count': implicit_pairs_cntr[-1][metric_id]}

def sample_table(args, hash_modulo=100000):
    train_remainder = min(hash_modulo, int(hash_modulo * args.train_fraction))
    if not args.is_test and train_remainder < hash_modulo or \
            args.is_test and train_remainder > 0:
        mapper = lambda row: sample_table_mapper(hash_modulo, train_remainder, args.is_test, row)
        yt.run_map(mapper, args.src, yt.TablePath(args.dst, sorted_by=['query_id']), spec={'data_size_per_job': 20 * 2**20, 'ordered': True})
        args.src = args.dst

def count_positive_pairs(key, rows):
    res = Counter()
    res['pos_reply'] = key['pos_reply']
    for row in rows:
        res['cnt_pos_pairs'] += int(row['weight'] != 0)
        res['cnt_pos_pairs_weighted'] += row['weight']
    yield res

def parse_target_params(priority, priority_weights, accept_thresholds, deny_thresholds):
    priority = [x.split(',') if x else [] for x in priority.split(';')]
    priority_lens = [len(x) for x in priority]
    priority_weights = [map(float, x.split(',')) if x else [1.] * priority_lens[i] for i, x in enumerate(priority_weights.split(';'))]
    accept_thresholds = [map(float, x.split(',')) if x else [0.] * priority_lens[i] for i, x in enumerate(accept_thresholds.split(';'))]
    deny_thresholds = [map(float, x.split(',')) if x else [0.] * priority_lens[i] for i, x in enumerate(deny_thresholds.split(';'))]
    assert all(l == len(priority_weights[i]) == len(accept_thresholds[i]) == len(deny_thresholds[i]) for i, l in enumerate(priority_lens))
    return priority, priority_weights, accept_thresholds, deny_thresholds

def print_pairs_statistics(priority, pairs_cntr):
    for table_id, table_priority in enumerate(priority):
        metrics = [name + '#' + str(i) for i, name in enumerate(table_priority)] + [name for (name, t) in pairs_cntr if t == table_id and '#' not in name]
        for name in metrics:
            if (name, table_id) not in pairs_cntr or sum(pairs_cntr[(name, table_id)].values()) == 0:
                continue
            print('%s\t%d (+%d, -%d)' % (name, pairs_cntr[(name, table_id)]['count'],
                                         pairs_cntr[(name, table_id)]['implicit_pos_count'],
                                         pairs_cntr[(name, table_id)]['implicit_neg_count']))
        if len(priority) > 1:
            print('total_%d\t%d' % (table_id, sum(v['count'] for k, v in pairs_cntr.items() if k[1] == table_id)))
            print('--------------------')
    print('total\t%d' % sum(v['count'] for v in pairs_cntr.values()))

def main(args):
    if not yt.get(args.src + '/@sorted') or yt.get(args.src + '/@sorted_by')[0] != 'query_id':
        yt.run_sort(args.src, args.dst, sort_by='query_id')
        args.src = args.dst

    sample_table(args)
    yt.run_map(map_sample, args.src, yt.TablePath(args.dst, sorted_by=['query_id']), spec={'data_size_per_job': 20 * 2**20, 'ordered': True})

    if args.mode == 'pairwise':
        assert args.dst_pairs
        priority, priority_weights, accept_thresholds, deny_thresholds = parse_target_params(args.priority, args.priority_weights, args.accept_thresholds, args.deny_thresholds)

        with yt.TempTable('//tmp') as dst_pairs_freq_table:
            yt.run_reduce(Reducer(priority, priority_weights, accept_thresholds, deny_thresholds, args.normalize_by_num_pairs, args.use_priority_weights_for_pair_count, args.s2s_sample_weight),
                args.dst, [yt.TablePath(args.dst, sorted_by=['key', 'id']), yt.TablePath(args.dst_pairs, sorted_by=['pos_id', 'neg_id']), dst_pairs_freq_table],
                reduce_by='query_id', spec={'job_io': {'control_attributes': {'enable_row_index': True}}})

            pairs_cntr = Counter()
            for row in yt.read_table(dst_pairs_freq_table):
                counts = {k: row[k] for k in ['count', 'implicit_pos_count', 'implicit_neg_count']}
                if (row['metric'], row['table_id']) not in pairs_cntr:
                    pairs_cntr[(row['metric'], row['table_id'])] = Counter()
                pairs_cntr[(row['metric'], row['table_id'])].update(counts)

        if args.dst_reply_freq:
            yt.run_sort(args.dst_pairs, args.dst_reply_freq, sort_by='pos_reply')
            yt.run_reduce(count_positive_pairs, args.dst_reply_freq, yt.TablePath(args.dst_reply_freq, sorted_by=['pos_reply']), reduce_by='pos_reply')

        print_pairs_statistics(priority, pairs_cntr)
    else:
        raise Exception('Unsupported mode: ' + args.mode)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', required=True)
    parser.add_argument('--priority', default='')
    parser.add_argument('--priority-weights', default='', help='Comma separated values')
    parser.add_argument('--accept-thresholds', default='', help='Comma separated values')
    parser.add_argument('--deny-thresholds', default='', help='Comma separated values')
    parser.add_argument('--use-priority-weights-for-pair-count', action='store_true')
    parser.add_argument('--normalize-by-num-pairs', default='none', help='One of { none, count, sqrt }')
    parser.add_argument('--s2s-sample-weight', type=float, default=1)
    parser.add_argument('--mode', default='pairwise')
    parser.add_argument('--dst', required=True)
    parser.add_argument('--dst-pairs', default=None)
    parser.add_argument('--dst-reply-freq', default=None)
    parser.add_argument('--train-fraction', type=float, default=1)
    parser.add_argument('--is-test', action='store_true')
    args = parser.parse_args()

    main(args)
