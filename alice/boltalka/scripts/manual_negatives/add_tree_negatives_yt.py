import yt.wrapper as yt
import argparse, os, sys
from copy import copy

yt.config.set_proxy("hahn.yt.yandex.net")


class Reducer(object):
    def __init__(self, n, num_rows):
        self.n = n
        self.num_rows = num_rows

    def __call__(self, key, rows):
        rows = list(rows)

        reply_user_id = {row['reply']: (row['user'], row['id']) for row in rows}
        reply_user = [(pair[0],pair[1][0]) for pair in sorted(reply_user_id.items(), key=lambda x: x[1][1])]
        root_user_id = {row['context_0']: (row['user_0'], row['id']) for row in rows if not row['context_0'] in reply_user_id}
        root_user = [(pair[0],pair[1][0]) for pair in sorted(root_user_id.items(), key=lambda x: x[1][1])]
        if len(root_user) > 1:
            print >> sys.stderr, len(root_user)

        all_candidates = reply_user + root_user

        for row in rows:
            row_id = row['id']
            positive_row = copy(row)
            positive_row['label'] = 1

            for i in xrange(self.n):
                positive_row['id'] = row_id + i*self.num_rows
                yield positive_row

            bad_replies = {x['reply'] for x in rows if x['context_0'] == row['context_0']} | {row['context_0']}
            good_candidates = [candidate for candidate in all_candidates if not candidate[0] in bad_replies]

            if len(good_candidates) == 0:
                #continue
                good_candidates = [(row['context_0'], row['user_0'])]

            negative_row = copy(row)
            negative_row['label'] = 0

            for i in xrange(self.n):
                negative_id = (row_id + i) % len(good_candidates)
                negative_row['reply'] = good_candidates[negative_id][0]
                negative_row['user'] = good_candidates[negative_id][1]
                negative_row['id'] = row_id + i*self.num_rows
                yield negative_row


def main(args):
    with yt.TempTable(os.path.dirname(args.src)) as src_sorted:
        yt.run_sort(args.src, src_sorted, sort_by=['key', 'id'])
        num_rows = yt.row_count(src_sorted)
        print 'finish sorting'
        yt.run_reduce(Reducer(args.n, num_rows), [src_sorted], [args.dst], reduce_by=['key'], sort_by=['key', 'id'])
    yt.run_sort(args.dst, sort_by=['id', 'label'])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', required=True)
    parser.add_argument('--dst', required=True)
    parser.add_argument('-n', type=int, default=5)
    args = parser.parse_args()
    main(args)