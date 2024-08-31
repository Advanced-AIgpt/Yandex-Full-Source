import yt.wrapper as yt
import argparse, sys, codecs, os

yt.config.set_proxy("hahn.yt.yandex.net")


def join_turns(row):
    turns = []
    for key in ['context_2', 'context_1', 'context_0']:
        turn = row[key]
        turn = turn.replace('\\n', '\n')
        turn = turn.replace('\\\\', '\\')
        turns.append(turn)
    row['context'] = '\t'.join(turns)
    del row['context_2'], row['context_1'], row['context_0']
    yield row


def reducer(key, rows):
    rows = list(rows)
    dct_upd = {}

    for row in rows:
        if 'result' in row:
            dct_upd.update({key: row[key] for key in ['result', 'probability']})
        elif 'male' in row:
            dct_upd.update({key: row[key] for key in ['male', 'male_prob',
                            'rude', 'rude_prob', 'you', 'you_prob']})

    if not 'result' in dct_upd:
        dct_upd.update({'result': '', 'probability': None})
    if not 'male' in dct_upd:
        dct_upd.update({'male': '', 'male_prob': None,
                        'rude': '', 'rude_prob': None,
                        'you': '', 'you_prob': None})

    for row in rows:
        if not ('result' in row or 'male' in row):
            row.update(dct_upd)
            yield row


def filter_unscored(row):
    if row['result'] == '' or row['male'] == '':
        return
    yield row


class Add_part_id(object):
    def __init__(self, part_id):
        self.part_id = part_id
    def __call__(self, row):
        row['part_id'] = self.part_id
        yield row


def concatenate(srces, dst):
    for src in yt.search(srces, node_type=["table"]):
        part_str = src.split('/')[-1]
        part_id = int(part_str[part_str.find('part')+4:])
        yt.run_map(Add_part_id(part_id), src, yt.TablePath(dst, append=True))


def add_part_id_to_src(table, part_size=10000):
    for row_idx, row in enumerate(yt.read_table(table)):
        row.update({'part_id': row_idx // part_size})
        yield row


def main(args):
    with yt.TempTable(os.path.dirname(args.src)) as temp_src, \
         yt.TempTable(os.path.dirname(args.src)) as temp_stage1, \
         yt.TempTable(os.path.dirname(args.src)) as temp_stage2:

        concatenate(args.stage1, temp_stage1)
        concatenate(args.stage2, temp_stage2)

        yt.run_map(join_turns, temp_stage1, temp_stage1)
        yt.run_sort(temp_stage1, temp_stage1, sort_by=['part_id', 'context', 'reply'])

        yt.run_map(join_turns, temp_stage2, temp_stage2)
        yt.run_sort(temp_stage2, temp_stage2, sort_by=['part_id', 'context', 'reply'])

        yt.write_table(temp_src, add_part_id_to_src(args.src))
        yt.run_sort(temp_src, temp_src, sort_by=['part_id', 'context', 'reply'])

        yt.run_reduce(reducer, [temp_src, '<foreign=%true>'+temp_stage1, '<foreign=%true>'+temp_stage2],
                      args.dst, join_by=['part_id', 'context', 'reply'], reduce_by=['part_id', 'context', 'reply'])

    num_rows = yt.row_count(args.dst)
    yt.run_map(filter_unscored, args.dst, args.dst)
    print '\n\n# unscored samples = %d\n\n' % (num_rows - yt.row_count(args.dst))
    yt.run_sort(args.dst, args.dst, sort_by=['context_id', 'inv_score'])



if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', required=True)
    parser.add_argument('--stage1', required=True)
    parser.add_argument('--stage2', required=True)
    parser.add_argument('--dst', required=True)
    args = parser.parse_args()
    main(args)
