from math import exp
import vh
import requests
import yt.wrapper as yt
import argparse
import json
import os
import getpass
from library.python import resource
from alice.boltalka.valhalla.utils import update_config, load_nirvana_data
from alice.boltalka.valhalla.operations import (
    get_mr_table,
    mr_sort,
    yt_cut,
    apply_dssm,
    find_top_replies_exact,
    postprocess_after_top_finding,
    select_querywise_top,
    yt_join,
    rerank,
    calc_factors,
    build_mx_pool,
    copy_column,
)

yt.config.set_proxy("hahn.yt.yandex.net")


session = requests.Session()
session.verify = False


def get_candidates(table, indexes, index_ids_display_rewritten, dssm_id=0):
    table = mr_sort(table, sort_by=['context_2', 'context_1', 'context_0'])
    weights = [0] * dssm_id + [1]
    parts = []
    for index in indexes:
        part = find_top_replies_exact(table, index, top_size=100, context_weight=0.5, dssm_weights=weights)
        part = yt_join(part, index, ('doc_id',), 'inner')
        parts.append(part)
    for index_id in map(int, index_ids_display_rewritten.split(',')):
        parts[index_id] = copy_column(parts[index_id], column_from='rewritten_reply', column_to='reply')
    table = mr_sort(parts, ['context_id', 'inv_score'])
    table = select_querywise_top(table, reduce_by=['context_id'], sort_by=['context_id', 'inv_score'], top_size=100)
    table = postprocess_after_top_finding(table, dssm_id)
    return table


def build_graph(args, config):
    data, updated_dssm_models = load_nirvana_data(config)
    index_names = []
    for k in ['index', 'index_1']:
        if k + '_apply' in config:
            index_names.append(k + '_apply')
        else:
            index_names.append(k)
    indexes = [get_mr_table(config[k]) for k in index_names]
    if updated_dssm_models:
        indexes = [apply_dssm(index, updated_dssm_models, lang=args.lang) for index in indexes]
    indexes = [mr_sort(index, ['shard_id', 'doc_id']) for index in indexes]
    src = get_mr_table(args.src)
    src = apply_dssm(src, data, lang=args.lang)
    tables = [get_candidates(src, indexes, args.index_ids_display_rewritten, dssm_id=dssm_id) for dssm_id in range(args.num_base_dssms)]
    table = mr_sort(tables, sort_by=['query_id'])
    table = calc_factors(table, data, num_base_dssms=args.num_base_dssms, lang=args.lang)
    table = mr_sort(table, sort_by=['query_id'])
    table, _ = rerank(table, data['mx_model'])
    return table


def main():
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--label', default='exact apply')
    parser.add_argument('--yt-token', required=True)
    parser.add_argument('--mr-account', default='tmp')
    parser.add_argument('--yt-proxy', default='hahn')
    parser.add_argument('--arcadia-revision')
    parser.add_argument('--project')
    parser.add_argument('--quota')
    parser.add_argument('--reuse-cache-policy', type=str, default='reuse_if_exist')
    parser.add_argument('--no-start', action='store_true')
    parser.add_argument('--src', required=True)
    parser.add_argument('--base-model')
    parser.add_argument('--factor-model')
    parser.add_argument('--rus-lister')
    parser.add_argument('--mx-model')
    parser.add_argument('--index')
    parser.add_argument('--index-1')
    parser.add_argument('--index-ids-display-rewritten', default='0')
    parser.add_argument('--num-base-dssms', type=int, default=2, help='model and first (num-base-dssms - 1) factor dssms are base (generating candidates)')
    parser.add_argument('--lang', default='ru')
    parser.add_argument('--config', default='ru_prod_cfg.json')
    args = parser.parse_args()

    assert args.config.startswith(args.lang)
    config = json.loads(resource.find(args.config))
    update_config(config, vars(args))

    with vh.Graph() as g, vh.NirvanaBackend():
        build_graph(args, config)

    vh.run(g, label=args.label, yt_token_secret=args.yt_token, mr_account=args.mr_account, yt_proxy=args.yt_proxy,
           project=args.project, quota=args.quota, start=not args.no_start,
           arcadia_revision=args.arcadia_revision, nirvana_cached_external_data_reuse_policy=args.reuse_cache_policy)
