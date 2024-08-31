# coding=utf-8
import argparse
import yt.wrapper as yt
import uuid
yt.config.set_proxy("hahn.yt.yandex.net")


class MxOpsParser(object):
    def __init__(self, inv_score_column, rank_column):
        self.inv_score_column = inv_score_column
        self.rank_column = rank_column

    def __call__(self, row):
        query_id = row['key']
        parts = row['value'].split('\t')
        rank = int(parts[2])
        score = float(parts[3])
        yield {'query_id': query_id,
               self.inv_score_column: -score,
               self.rank_column: rank,
               }


@yt.with_context
class InvScoreJoiner(object):
    def __init__(self, inv_score_column, rank_column):
        self.inv_score_column = inv_score_column
        self.rank_column = rank_column

    def __call__(self, key, rows, context):
        rank2score = {}
        rank = 0
        for row in rows:
            if context.table_index == 0:
                rank2score[row[self.rank_column]] = row[self.inv_score_column]
            else:
                row[self.inv_score_column] = rank2score[rank]
                yield row
                rank += 1
        assert rank == len(rank2score)


def main(args):
    assert yt.get(args.src + '/@sorted') and yt.get(args.src + '/@sorted_by')[0] == 'query_id'
    rank_column = uuid.uuid4().hex
    row = next(yt.read_table(args.src))
    assert rank_column not in row and args.inv_score_column not in row

    yt.run_map(MxOpsParser(args.inv_score_column, rank_column), args.mx_ops, yt.TablePath(args.dst, sorted_by=['query_id']), ordered=True)
    yt.run_reduce(InvScoreJoiner(args.inv_score_column, rank_column), [args.dst, args.src], args.dst, reduce_by='query_id')
    yt.run_sort(args.dst, sort_by=['query_id', args.inv_score_column])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', required=True, help='input of build_mx_pool')
    parser.add_argument('--mx-ops', required=True)
    parser.add_argument('--inv-score-column', default='inv_reranker_score')
    parser.add_argument('--dst', required=True)
    args = parser.parse_args()
    main(args)
