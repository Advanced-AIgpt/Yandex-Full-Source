import argparse
import yt.wrapper as yt

yt.config.set_proxy("hahn.yt.yandex.net")


class Dialogues_builder(object):
    def __init__(self, context_length):
        self.context_length = context_length
    def __call__(self, row):
        turns = [''] * self.context_length + row['value'].split('\t') + ['']
        num_turns = len(turns) - self.context_length
        for i in xrange(num_turns):
            dialogue = '\t'.join(turns[i:i+self.context_length+1])
            yield {'key': row['key'], 'value': dialogue, 'depth': i, 'height': num_turns - i - 2}


def remove_duplicates(key, rows):
    prev_value = None
    rows = list(rows)
    num_rows = len(rows)
    value2height_dct = {}
    for row in rows:
        value2height_dct[row['value']] = min(value2height_dct.get(row['value'], num_rows), row['height'])
    res = {}
    res['key'] = key['key']

    for row in rows:
        if row['value'] == prev_value:
            continue
        res['depth'] = row['depth']
        res['height'] = value2height_dct[row['value']]
        user_ids, turns = [], []
        for message in row['value'].split('\t'):
            if message == '':
                user_ids.append(None)
                turns.append(None)
            else:
                parts = message.split(' ')
                user_ids.append(parts[0])
                turns.append(' '.join(parts[1:]))
        res['reply'] = turns[-1]
        res['user'] = user_ids[-1]
        context_length = len(turns) - 1
        for i in xrange(context_length):
            j = context_length - i - 1
            res['context_%d' % j] = turns[i]
            res['user_%d' % j] = user_ids[i]
        yield res
        prev_value = row['value']


def add_width(key, rows):
    rows = list(rows)
    if key['depth'] == -1:
        assert len(rows) == 1
        width = 0
    else:
        width = len(rows)
    for row in rows:
        row['width'] = width
        yield row


def main(args):
    yt.run_map_reduce(Dialogues_builder(args.context_length), remove_duplicates, args.src, args.dst,
                      reduce_by=['key'], sort_by=['key', 'value'])
    reduce_cols = ['key', 'depth', 'user_2', 'context_2', 'user_1', 'context_1', 'user_0', 'context_0']
    yt.run_sort(args.dst, sort_by=reduce_cols)
    yt.run_reduce(add_width, args.dst, args.dst, reduce_by=reduce_cols)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', required=True)
    parser.add_argument('--dst', required=True)
    parser.add_argument('--context-length', type=int, default=3)
    args = parser.parse_args()
    main(args)
