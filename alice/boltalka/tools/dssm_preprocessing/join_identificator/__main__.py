import yt.wrapper as yt
import argparse, hashlib

yt.config.set_proxy("hahn.yt.yandex.net")

@yt.with_context
class Identifier(object):
    def __init__(self, col):
        self.col = col

    def __call__(self, row, context):
        key = str(context.row_index)
        for turn in ['reply', 'context_0', 'context_1', 'context_2']:
            text = row[turn] if row[turn] is not None else ''
            key += '\t' + text
        row[self.col] = hashlib.md5(key).hexdigest()
        yield row


def main(args):
    yt.run_map(Identifier(args.col), args.src, args.dst, spec={'job_io': {'control_attributes': {'enable_row_index': True}}})
    yt.run_sort(args.dst, sort_by=[args.col])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', required=True)
    parser.add_argument('--dst', required=True)
    parser.add_argument('--col', required=True)
    args = parser.parse_args()
    main(args)
