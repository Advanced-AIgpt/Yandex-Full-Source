import yt.wrapper as yt
import argparse, os

yt.config.set_proxy("hahn.yt.yandex.net")


def split_turns(row):
    for prefix in ['context', 'reply_context']:
        parts = row[prefix].split('\t')
        for idx, part in enumerate(parts[::-1]):
            row[prefix + '_%d' % idx] = part
        del row[prefix]
    yield row


def join_turns(row):
    for prefix in ['context', 'reply_context']:
        parts = []
        for idx in reversed(range(3)):
            parts.append(row[prefix + '_%d' % idx])
            del row[prefix + '_%d' % idx]
        row[prefix] = '\t'.join(parts)
    yield row


class Pool_frequency_getter(object):
    def __init__(self, name, suffix='_poolfreq'):
        self.name = name
        self.suffix = suffix
    def __call__(self, key, rows):
        freq = sum(1 for _ in rows)
        yield {self.name: key[self.name], self.name + self.suffix: freq}


class Rename_col(object):
    def __init__(self, old_name, new_name):
        self.old_name = old_name
        self.new_name = new_name
    def __call__(self, row):
        row[self.new_name] = row[self.old_name]
        del row[self.old_name]
        yield row


class Joiner(object):
    def __init__(self, name, suffix='_poolfreq'):
        self.name = name + suffix

    def __call__(self, key, rows):
        freq = 0
        for row in reversed(list(rows)):
            if self.name in row:
                freq = row[self.name]
            else:
                row[self.name] = freq
                yield row


def main(args):
    is_context_splitted = 'context_0' in next(yt.read_table(args.src))
    if not is_context_splitted:
        yt.run_map(split_turns, args.src, args.dst)

    with yt.TempTable(os.path.dirname(args.src)) as temp:
        pool = args.pool
        for name in ['context_0', 'reply', 'reply_context_0']:
            if name == 'reply_context_0':
                yt.run_map(Rename_col('context_0', 'reply_context_0'), pool, temp)
                pool = temp
            yt.run_sort(pool, temp, sort_by=[name])
            yt.run_reduce(Pool_frequency_getter(name), temp, temp, reduce_by=[name])
            yt.run_sort(temp, temp, sort_by=[name])
            yt.run_sort(args.dst, args.dst, sort_by=[name])
            yt.run_reduce(Joiner(name), [args.dst, '<foreign=%true>'+temp], args.dst,
                          join_by=[name], reduce_by=[name], sort_by=[name])

    if not is_context_splitted:
        yt.run_map(join_turns, args.dst, args.dst)



if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', required=True)
    parser.add_argument('--pool', required=True)
    parser.add_argument('--dst', required=True)
    args = parser.parse_args()
    main(args)
