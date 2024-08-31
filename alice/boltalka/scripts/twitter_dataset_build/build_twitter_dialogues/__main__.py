import argparse
import yt.wrapper as yt
import hashlib

yt.config.set_proxy("hahn.yt.yandex.net")


class Dialogues_builder(object):
    def __init__(self, context_length):
        self.context_length = context_length
    def __call__(self, row):
        turns = [''] * self.context_length + row['value'].split('\t') + ['']
        langs = [''] * self.context_length + row['langs'].split('\t') + [''] if 'langs' in row else None
        num_turns = len(turns) - self.context_length
        for i in range(num_turns):
            dialogue = '\t'.join(turns[i:i+self.context_length+1])
            hashcode = hashlib.md5(dialogue).hexdigest()
            row.update({'value': dialogue, 'depth': i, 'height': num_turns - i - 2, 'inv_depth': -i, 'hashcode': hashcode})
            if langs is not None:
                row['langs'] = '\t'.join(langs[i:i+self.context_length+1])
            yield row


def parse_dialogue(dialogue):
    user_ids, texts = [], []
    for turn in dialogue.split('\t'):
        if turn == '':
            user_ids.append(None)
            texts.append(None)
        else:
            parts = turn.split(' ')
            user_ids.append(parts[0])
            texts.append(' '.join(parts[1:]))
    dct = {'reply': texts[-1], 'user': user_ids[-1]}
    context_length = len(texts) - 1
    for i in range(context_length):
        j = context_length - i - 1
        dct['context_%d' % j] = texts[i]
        dct['user_%d' % j] = user_ids[i]
    return dct


def remove_duplicates(key, rows):
    row = next(rows)
    dct_upd = parse_dialogue(row['value'])
    del row['value'], row['inv_depth']
    row.update(dct_upd)
    yield row


class AddReduceKey(object):
    def __init__(self, columns):
        self.columns = columns
    def __call__(self, row):
        key = '\t'.join(str(row[k]) for k in self.columns)
        row['reduce_key'] = hashlib.md5(key).hexdigest()
        yield row


def add_width(key, rows):
    rows = list(rows)
    if rows[0]['depth'] == -1:
        assert len(rows) == 1
        width = 0
    else:
        width = len(rows)
    for row in rows:
        row['width'] = width
        del row['reduce_key']
        yield row


def main(args):
    yt.run_map_reduce(Dialogues_builder(args.context_length), remove_duplicates, args.src, args.dst,
                      reduce_by=['hashcode'], sort_by=['hashcode', 'inv_depth', 'key'])
    reduce_cols = ['key', 'depth']
    for i in range(args.context_length):
        reduce_cols.extend(['context_' + str(i), 'user_' + str(i)])
    yt.run_map_reduce(AddReduceKey(reduce_cols), add_width, args.dst, args.dst, reduce_by='reduce_key')
    yt.run_sort(args.dst, sort_by='key')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', required=True)
    parser.add_argument('--dst', required=True)
    parser.add_argument('--context-length', type=int, default=3)
    args = parser.parse_args()
    main(args)
