# coding=utf-8
import yt.wrapper as yt
import argparse, os

yt.config.set_proxy("hahn.yt.yandex.net")


def join_turns(row):
    for prefix in ['context', 'reply_context']:
        parts = []
        for idx in reversed(range(3)):
            parts.append(row[prefix + '_%d' % idx])
            del row[prefix + '_%d' % idx]
        row[prefix] = '\t'.join(parts)
    yield row


class Replies_merger(object):
    def __init__(self, num_top_replies):
        self.num_top_replies = num_top_replies

    def __call__(self, key, rows):
        rows = list(rows)[::-1]
        top_rows = []
        for top_idx in range(len(rows)):
            if 'context_id' in rows[top_idx]:
                top_rows.append({col: rows[top_idx][col]
                                 for col in ['reply_context', 'reply', 'score', 'inv_score']})
            else:
                break

        top_rows = top_rows[:self.num_top_replies]

        for src_idx in range(top_idx, len(rows)):
            row = rows[src_idx]
            row['inv_score'] = -row['score']
            for top_row in top_rows:
                top_row['feedback'] = 0.5
                if top_row['reply'] != row['reply']:
                    yield row
                    row.update(top_row)
                    yield row



def main(args):
    with yt.TempTable(os.path.dirname(args.src)) as temp_src, \
         yt.TempTable(os.path.dirname(args.src)) as temp_top:

        yt.run_sort(args.top, temp_top, sort_by=['context'])

        yt.run_map(join_turns, args.src, temp_src)
        yt.run_sort(temp_src, temp_src, sort_by=['context'])

        yt.run_reduce(Replies_merger(args.num_top_replies), [temp_src, '<foreign=%true>'+temp_top], args.dst,
                      join_by=['context'], reduce_by=['context'], sort_by=['context'],
                      memory_limit=10000000000)

    yt.run_sort(args.dst, args.dst, sort_by=['id', 'inv_score'])



if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', required=True)
    parser.add_argument('--top', required=True)
    parser.add_argument('--num-top-replies', type=int, default=1)
    parser.add_argument('--dst', required=True)
    args = parser.parse_args()
    main(args)
