import argparse
import yt.wrapper as yt


yt.config.set_proxy("hahn.yt.yandex.net")


def generate_dialogs(row):
    turns = ['']*2 + row['value'].split('\t')
    for i in xrange(len(turns)-3):
        yield {'key': row['key'], 'value': '\t'.join(turns[i:i+4])}


def make_unique(key, rows):
    for dialog in {row['value'] for row in rows}:
        ids, turns = [], []
        for message in dialog.split('\t'):
            parts = message.split(' ')
            ids.append(parts[0])
            turns.append(' '.join(parts[1:]))
        yield dict(zip(['key','context_2','context_1','context_0','reply', 'user_2', 'user_1', 'user_0', 'user'], [str(key['key'])]+turns+ids))


def main(args):
    yt.run_map_reduce(generate_dialogs, make_unique, [args.src], [args.dst], reduce_by=['key'])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', required=True)
    parser.add_argument('--dst', required=True)
    args = parser.parse_args()
    main(args)
