import vh
import requests
import yt.wrapper as yt
import argparse
import os
import getpass
import itertools
from alice.boltalka.valhalla.operations import (
    add_query_id,
    get_mr_table,
    mr_sort,
    mr_merge,
    mr_move,
    yt_cut,
    yt_join,
    select_rows_by_value,
    mr_read_tsv,
    remove_duplicates,
)

from alice.boltalka.valhalla.lib.toloka import (
    add_toloka_keys,
    make_toloka_input,
    boltalka_nature_evaluation,
    boltalka_properties_evaluation,
    boltalka_interest_evaluation,
)

yt.config.set_proxy("hahn.yt.yandex.net")


session = requests.Session()
session.verify = False


def preprocess_assessment_mapper(row):
    for i in range(9):
        turn = row['context_' + str(i)]
        del row['context_' + str(i)]
        turn = turn.replace('\\n', '\n').replace('\\\\', '\\')
        row['query_' + str(i)] = turn
    row['reply'] = row['reply'].replace('\\n', '\n').replace('\\\\', '\\')
    yield row

@vh.lazy.hardware_params(vh.HardwareParams(max_ram=1000))
@vh.lazy(input_table=vh.YTTable, output_table=vh.mkoutput(vh.YTTable))
def preprocess_assessment(input_table, output_table):
    yt.run_map(preprocess_assessment_mapper, input_table, output_table)
    return output_table


class PreprocessTopsMapper(object):
    def __call__(self, row):
        row['reply'] = row['rewritten_reply']
        yield row

@vh.lazy.hardware_params(vh.HardwareParams(max_ram=1000))
@vh.lazy(input_table=vh.YTTable, output_table=vh.mkoutput(vh.YTTable))
def preprocess_tops(input_table, output_table):
    yt.run_map(PreprocessTopsMapper(), input_table, output_table)
    return output_table


class AddColumnMapper(object):
    def __init__(self, column, value):
        self.column = column
        self.value = value

    def __call__(self, row):
        assert self.column not in row
        row[self.column] = self.value
        yield row

@vh.lazy.hardware_params(vh.HardwareParams(max_ram=1000))
@vh.lazy(input_table=vh.YTTable, output_table=vh.mkoutput(vh.YTTable), column=vh.mkoption(str), value=vh.mkoption(int))
def add_column(input_table, column, value, output_table):
    yt.run_map(AddColumnMapper(column, value), input_table, output_table)
    return output_table


JOIN_COLUMNS = ['reply'] + ['query_{}'.format(i) for i in range(9)]


def get_parts(prefix):
    data = []
    for i in itertools.count():
        path = prefix + str(i)
        if not yt.exists(path):
            break
        data.append(get_mr_table(path))
    return data


def join_assesments(tops, parts, name, columns):
    data = mr_merge(parts)
    data = preprocess_assessment(data)
    data = yt_cut(data, columns=JOIN_COLUMNS + columns)
    data = remove_duplicates(data, reduce_by=JOIN_COLUMNS)
    return yt_join(tops, data, by=JOIN_COLUMNS, type='inner')


def build_graph(args):
    tops = get_mr_table(args.src)
    tops = add_query_id(tops)
    nature_assessment_prefix = os.path.join(args.dst_assessment, 'nature', 'part')
    properties_assessment_prefix = os.path.join(args.dst_assessment, 'properties', 'part')
    interest_assessment_prefix = os.path.join(args.dst_assessment, 'interest', 'part')
    boltalka_nature_evals = []
    boltalka_properties_evals = []
    interest_evals = []
    if args.mode == 'assess':
        after = []
        for part_idx in range(args.num_parts):
            tops_part = select_rows_by_value(tops, column='query_id', value=part_idx, modulo=args.num_parts)
            tsv = mr_read_tsv(tops_part, columns=['query_{}'.format(i) for i in reversed(range(9))] + ['rewritten_reply', 'query_id'], escaping=True)
            json = add_toloka_keys(make_toloka_input(tsv, context_len=9))
            boltalka_nature_evals.append(boltalka_nature_evaluation(json, args.priority, nature_assessment_prefix + str(i), after=after, mr_account=args.caching_mr_account))
            boltalka_properties_evals.append(boltalka_properties_evaluation(json, args.priority, properties_assessment_prefix + str(i), after=after, mr_account=args.caching_mr_account))
            interest_evals.append(boltalka_interest_evaluation(tops_part, interest_assessment_prefix + str(part_idx), after=after))
            after = [boltalka_nature_evals[-1], boltalka_properties_evals[-1], interest_evals[-1]]
            after = [interest_evals[-1]]
        boltalka_nature_evals = [el['table'] for el in boltalka_nature_evals]
        boltalka_properties_evals = [el['table'] for el in boltalka_properties_evals]
        interest_evals = [el['table'] for el in interest_evals]
    elif args.mode == 'gather':
        boltalka_nature_evals = get_parts(nature_assessment_prefix)
        boltalka_properties_evals = get_parts(properties_assessment_prefix)
        interest_evals = get_parts(interest_assessment_prefix)

    tops = join_assesments(tops, boltalka_nature_evals, 'nature', ['result', 'probability'])
    tops = join_assesments(tops, boltalka_properties_evals, 'properties', ['male', 'male_prob', 'rude', 'rude_prob', 'you', 'you_prob'])
    tops = join_assesments(tops, interest_evals, 'interest', ['mark'])
    tops = mr_sort(tops, ['query_id', 'inv_score'])
    mr_move(tops, args.dst)


def main():
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--label', default='assess pool')
    parser.add_argument('--yt-token', required=True)
    parser.add_argument('--mr-account', default='tmp')
    parser.add_argument('--caching-mr-account', default='voice')
    parser.add_argument('--yt-proxy', default='hahn')
    parser.add_argument('--arcadia-revision')
    parser.add_argument('--project')
    parser.add_argument('--quota')
    parser.add_argument('--reuse-cache-policy', type=str, default='reuse_if_exist')
    parser.add_argument('--no-start', action='store_true')
    parser.add_argument('--mode', default='assess', choices=['assess', 'gather'])
    parser.add_argument('--src', required=True)
    parser.add_argument('--dst', required=True)
    parser.add_argument('--dst-assessment', required=True)
    parser.add_argument('--num-parts', type=int, default=10)
    parser.add_argument('--priority', type=int, default=70)
    args = parser.parse_args()

    with vh.Graph() as g, vh.NirvanaBackend():
        build_graph(args)

    vh.run(g, label=args.label, yt_token_secret=args.yt_token, mr_account=args.mr_account, yt_proxy=args.yt_proxy,
           project=args.project, quota=args.quota, start=not args.no_start,
           arcadia_revision=args.arcadia_revision, nirvana_cached_external_data_reuse_policy=args.reuse_cache_policy)
