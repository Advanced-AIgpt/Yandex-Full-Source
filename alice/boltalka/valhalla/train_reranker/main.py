import vh
import requests
import yt.wrapper as yt
import argparse
import itertools
import copy
import yaml
from library.python import resource
from alice.boltalka.valhalla.utils import update_config, load_nirvana_data
from alice.boltalka.valhalla.operations import (
    get_mr_table,
    mr_sort,
    mr_merge,
    mr_move,
    yt_join,
    calc_reranker_metrics,
    get_catboost_column_description,
    convert_cb2mxnet,
    calc_factors,
    train_test_split,
    build_mx_pool,
    train_catboost,
    add_query_id,
    select_rows_by_range,
    select_rows_by_value,
    select_rows_by_text_column,
    rewrite_replies,
    punctuate_replies,
    add_more_respect,
    apply_dssm,
    rerank,
    preprocess_mx_pairs,
    copy_column,
    delete_columns,
    normalize_columns,
    remove_duplicates,
    cache_table,
    calc_informativeness,
    draw_curve,
    add_str_column,
    mx_ops_calc,
)
from alice.boltalka.valhalla.train_reranker.factors.main import (
    edit_features,
    calc_num_factors,
    reweigh_pairs_by_freq,
    SELECTIVE_MODEL_NAME,
    SEQ2SEQ_MODEL_NAME,
    INFORMATIVENESS_DENOMINATOR_STATIC_FACTOR_COLUMN,
    INFORMATIVENESS_MODEL,
    quantize_train_and_validation,
    set_context_length,
    EMBEDDING_PREFIXES,
    add_func_factors,
)
from alice.boltalka.valhalla.train_reranker.seq2seq.main import (
    zero_context_embeddings,
    mock_selective_columns,
)
from alice.boltalka.valhalla.train_reranker.reporting.main import (
    make_total_report,
)
from alice.boltalka.valhalla.lib.pulsar.__init__ import (
    get_report_metrics,
    add_update_pulsar_instance,
)
yt.config.set_proxy("hahn.yt.yandex.net")


session = requests.Session()
session.verify = False


def add_is_valid_column(pool_names, validation_pool_names, train_fraction, split_by='query_id', split_value_base=16):
    non_val_pools = pool_names - validation_pool_names
    if non_val_pools:
        non_val = mr_merge([get_mr_table(name) for name in non_val_pools])
        non_val = add_query_id(non_val)
    if validation_pool_names:
        validation_source = mr_merge([get_mr_table(name) for name in validation_pool_names])
        validation_source = add_query_id(validation_source)
        valid = train_test_split(validation_source, split_by, split_value_base, train_fraction, is_test=True)
        valid = add_str_column(valid, 'is_valid', '1')
        train = train_test_split(validation_source, split_by, split_value_base, train_fraction, is_test=False)
        if non_val_pools:
            train = mr_merge([train, non_val])
    else:
        train = non_val
    train = add_str_column(train, 'is_valid', '0')
    return mr_merge([train, valid])


def make_rewritten_reply(table, lang, revision):
    if lang != 'ru':
        table = copy_column(table, column_from='reply', column_to='rewritten_reply')
        table = normalize_columns(table, ['context_2', 'context_1', 'context_0', 'rewritten_reply'])
        return table
    table = normalize_columns(table, ['reply'])
    table = delete_columns(table, ['rewritten_reply'])
    table = rewrite_replies(table, revision)
    table = punctuate_replies(table)
    table = add_more_respect(table, revision)
    table = normalize_columns(table, ['rewritten_reply'])
    return table


def preprocess_selective(data, config, dssm_models, args, negative_contexts):
    table = make_rewritten_reply(data, args.lang, args.arcadia_revision)
    table = add_str_column(table, 'model_type', SELECTIVE_MODEL_NAME)

    parts = []
    index_names_display_rewritten = set(args.index_names_display_rewritten.split(','))

    for index_name, index_data in config['indexes'].items():
        index = get_mr_table(index_data['table'])
        index = make_rewritten_reply(index, args.lang, args.arcadia_revision)
        index = remove_duplicates(index, reduce_by=['rewritten_reply', 'context_0', 'context_1', 'context_2'],
                                  sort_by=['rewritten_reply', 'context_0', 'context_1', 'context_2', 'shard_id', 'doc_id'])
        part = select_rows_by_text_column(table, column='index_name', value=index_name)
        part = yt_join(part, index, ['context_0', 'context_1', 'context_2', 'rewritten_reply'], 'inner')
        part = copy_column(part, column_from='reply', column_to='_reply')
        if index_name in index_names_display_rewritten:
            part = copy_column(part, column_from='rewritten_reply', column_to='reply')
        if index_data.get('index_type') == 'reply':
            part = zero_context_embeddings(part)
        parts.append(part)
    table = mr_merge(parts)
    table = cache_table(table, mr_account=args.caching_mr_account)
    if args.updated_dssm_model_names:
        updated_dssm_models = {k: dssm_models[k] for k in args.updated_dssm_model_names}
        table = apply_dssm(table, updated_dssm_models, lang=args.lang, reply_column='reply' if args.apply_dssm_to_rewritten else '_reply')
    if 'base_model' in args.updated_dssm_model_names:
        table = calc_informativeness(table, negative_contexts, denominator_column=INFORMATIVENESS_DENOMINATOR_STATIC_FACTOR_COLUMN)
    return table


def preprocess_s2s(data, config, dssm_models, args, negative_contexts):
    num_static_factors = calc_num_factors(config['indexes']['base']['table'], prefix='static_')
    table = mock_selective_columns(data, num_static_factors)
    table = add_str_column(table, 'model_type', SEQ2SEQ_MODEL_NAME)
    table = copy_column(table, column_from='reply', column_to='_reply')
    table = cache_table(table, mr_account=args.caching_mr_account)
    table = apply_dssm(table, dssm_models, lang=args.lang, reply_column='_reply')
    table = calc_informativeness(table, negative_contexts, denominator_column=INFORMATIVENESS_DENOMINATOR_STATIC_FACTOR_COLUMN)
    table = zero_context_embeddings(table)
    return table


def preprocess(data, config, dssm_models, args, negative_contexts):
    selective = select_rows_by_value(data, 'is_s2s', 0)
    s2s = select_rows_by_value(data, 'is_s2s', 1)
    return mr_merge([
        preprocess_selective(selective, config, dssm_models, args, negative_contexts),
        preprocess_s2s(s2s, config, dssm_models, args, negative_contexts)
    ])


def get_negative_contexts(config, dssm_models, inf_dssm_model, lang):
    negative_contexts = get_mr_table(config['inf_negatives_table'])
    dssms_for_negatives = dict(dssm_models)
    dssms_for_negatives[INFORMATIVENESS_MODEL] = inf_dssm_model
    negative_contexts = apply_dssm(negative_contexts, dssms_for_negatives, lang=lang)
    return negative_contexts


MODEL_KEYS = ['base_model', 'factor_model', 'factor_model_1', 'factor_model_2']


def build_train_and_validation(args, config):
    lang = config['lang']
    dssm_models = {}
    args.updated_dssm_model_names = set()
    for k, model in zip(MODEL_KEYS, config['shard_config']['models']):
        model_id = model['id']
        if vars(args).get(k):
            model_id = vars(args)[k]
            args.updated_dssm_model_names.add(k)
        dssm_models[k] = vh.data(id=model_id)

    inf_dssm_model = vh.data(id=config['inf_model'])
    inf_negatives = get_negative_contexts(config, dssm_models, inf_dssm_model, lang)

    pools = set(config['pools'])
    validation_pools = set(config['validation_pools'])
    excluded_pools = set(args.exclude_pools.split(','))
    pools -= excluded_pools
    validation_pools -= excluded_pools

    full_data = add_is_valid_column(pools, validation_pools, args.train_fraction)

    full_data = preprocess(full_data, config, dssm_models, args, inf_negatives)

    full_data = apply_dssm(full_data, {INFORMATIVENESS_MODEL: inf_dssm_model}, lang=lang,
                       context_column_prefix='query_', reply_column='reply' if args.apply_dssm_to_rewritten else '_reply')
    full_data = calc_informativeness(full_data, inf_negatives, informativeness_column='target_inf_indep',
                                 reply_embedding_column=EMBEDDING_PREFIXES[INFORMATIVENESS_MODEL]+'reply_embedding',
                                 context_embedding_column=EMBEDDING_PREFIXES[INFORMATIVENESS_MODEL]+'context_embedding',
                                 negative_embedding_column=EMBEDDING_PREFIXES[INFORMATIVENESS_MODEL]+'context_embedding')

    full_data = mr_sort(full_data, sort_by='query_id')
    rus_lister = vh.data(id=config['shard_config']['ruslister'])
    full_data = calc_factors(full_data, dssm_models, rus_lister, num_base_dssms=args.num_base_dssms, lang=lang)
    full_data = edit_features(full_data, lang)

    if args.quantization_settings is not None:
        full_data = quantize_train_and_validation(full_data, args.quantization_settings)

    full_data = cache_table(full_data, mr_account=args.caching_mr_account)
    if args.precalced_factors:
        full_data = mr_move(full_data, args.precalced_factors, force=True)
    return full_data


def build_catboost_dataset(pool, target_priority, target_weights, accept_thresholds, deny_thresholds, pair_weight_gamma, reply_freq=None):
    mx_pool, pairs, new_reply_freq = build_mx_pool(pool, target_priority, target_weights, accept_thresholds, deny_thresholds)
    if not reply_freq:
        reply_freq = new_reply_freq
    pairs = preprocess_mx_pairs(reweigh_pairs(pairs, reply_freq, gamma=pair_weight_gamma))
    return mx_pool, pairs, reply_freq


def reweigh_pairs(pairs, reply_freq, gamma):
    pairs = yt_join(pairs, reply_freq, by='pos_reply', type='left')
    pairs = reweigh_pairs_by_freq(pairs, gamma)
    return pairs


def run_async(g, args, label_prefix=''):
    label = args.label
    if label_prefix:
        label = label_prefix + ' ' + label
    return vh.run_async(g, label=label, yt_token_secret=args.yt_token, mr_account=args.mr_account, yt_pool=args.yt_pool,
        yt_proxy=args.yt_proxy, project=args.project, quota=args.quota, start=True, keep_going=not args.not_keep_going,
        nirvana_cached_external_data_reuse_policy=args.reuse_cache_policy, nirvana_result_cloning_policy=args.nirvana_result_cloning_policy,
        clone_workflow_instance=args.clone_workflow_instance, clone_to_new_workflow=args.clone_to_new_workflow,
        lazy_deploy_type='separate')


def train_reranker(args, config):
    args.lang = config['lang']

    if args.precalced_factors is None:
        table = build_train_and_validation(args, config)
    else:
        if ',' in args.precalced_factors:
            table = mr_merge([add_str_column(get_mr_table(t), 'table_id', str(i)) for i, t in enumerate(args.precalced_factors.split(','))])
        else:
            table = mr_merge(get_mr_table(args.precalced_factors))

    func_factors = args.func_factors.split(',') if args.func_factors else []
    table = add_func_factors(table, func_factors)

    train_pool = mr_sort(select_rows_by_text_column(table, column='is_valid', value='0'), sort_by=['query_id', 'inv_score'])
    validation_pool = mr_sort(select_rows_by_text_column(table, column='is_valid', value='1'), sort_by=['query_id', 'inv_score'])


    mx_models = {}
    total_reports = []

    def iterate_target_params(params):
        if params is None:
            yield None
            return
        parsed = [x.split(':') for x in params.split(',')]
        for x in itertools.product(*parsed):
            yield ','.join(x)
    external_validation = args.external_validation.split(',') if args.external_validation else []
    ext_validation_pools = [
        mr_sort(add_func_factors(select_rows_by_text_column(get_mr_table(pool), column='is_valid', value='1'), func_factors), sort_by=['query_id', 'inv_score'])
        for pool in external_validation
    ]
    if args.like_prod_validation:
        ext_validation_pools.append(mr_sort(add_func_factors(get_mr_table(args.like_prod_validation), func_factors), sort_by=['query_id', 'inv_score']))
    ext_reports = [
        [] for _ in ext_validation_pools
    ]
    for idx, (target_weights, accept_thresholds, deny_thresholds, pair_weight_gamma) in enumerate(itertools.product(iterate_target_params(args.target_weights),
                                                                                                   iterate_target_params(args.accept_thresholds),
                                                                                                   iterate_target_params(args.deny_thresholds),
                                                                                                   map(float, args.pair_weight_gamma.split(':')))):
        model_spec = {
            'target_priority': args.target_priority,
            'target_weights': target_weights,
            'accept_thresholds': accept_thresholds,
            'deny_thresholds': deny_thresholds,
            'pair_weight_gamma': pair_weight_gamma,
        }
        train_mx_pool, train_pairs, train_reply_freq = build_catboost_dataset(train_pool, **model_spec)
        validation_mx_pool, validation_pairs, _ = build_catboost_dataset(validation_pool, reply_freq=train_reply_freq, **model_spec)
        ext_validation_mx_pools = [build_mx_pool(pool)[0] for pool in ext_validation_pools]
        if idx == 0:
            catboost_cd = get_catboost_column_description(validation_mx_pool)
            if 'result' in args.target_priority.split(','):
                calc_reranker_metrics(validation_mx_pool, rerank_by_targets='result')
                calc_reranker_metrics(validation_mx_pool, rerank_by_targets='result', inverse=True)
            if 'target_bert' in args.target_priority.split(','):
                calc_reranker_metrics(validation_mx_pool, rerank_by_targets='target_bert')

        for lr in map(float, args.lr.split(':')):
            model_spec['lr'] = lr
            catboost_trainer = train_catboost(catboost_cd, train_mx_pool, train_pairs, validation_mx_pool, validation_pairs, lr, args.num_iter, args.calc_fstr)
            model_name = ';'.join(k+'='+str(v) for k, v in model_spec.items())
            mx_models[model_name] = convert_cb2mxnet(catboost_trainer['model.bin'])
            if args.rerank_validation:
                _, ops = rerank(validation_pool, mx_models[model_name], validation_mx_pool)
            else:
                ops = mx_ops_calc(mx_models[model_name], validation_mx_pool)
            total_reports.append(calc_reranker_metrics(ops, catboost_trainer['training_log.json'], model_name=model_name)['output_json'])

            for i, (reports, pool, mx_pool) in enumerate(zip(ext_reports, ext_validation_pools, ext_validation_mx_pools)):
                if args.rerank_validation:
                    _, ops = rerank(pool, mx_models[model_name], mx_pool)
                else:
                    ops = mx_ops_calc(mx_models[model_name], mx_pool)
                if args.like_prod_validation and i == len(ext_validation_pools) - 1:
                    reports.append(calc_reranker_metrics(ops, catboost_trainer['training_log.json'], model_name=model_name, top='10', unique_top=True)['output_json'])
                else:
                    reports.append(calc_reranker_metrics(ops, catboost_trainer['training_log.json'], model_name=model_name)['output_json'])

            def filter_config(config):
                return {str(k): str(v) for k, v in config.items() if k[0] != '_'}

    total_report, _ = make_total_report(total_reports)
    for reports in ext_reports:
        make_total_report(reports)
    if args.draw_curve and args.curve_axis:
        x_name, y_name = args.curve_axis.split(',')
        draw_curve(total_report, x_name, y_name)


def main():
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--label', default='train reranker')
    parser.add_argument('--yt-token', required=True)
    parser.add_argument('--mr-account', default='tmp')
    parser.add_argument('--yt-pool', default='dialogs')
    parser.add_argument('--caching-mr-account', default='dialogs')
    parser.add_argument('--yt-proxy', default='hahn')
    parser.add_argument('--arcadia-revision', type=int, default=6484404)
    parser.add_argument('--project')
    parser.add_argument('--quota')
    parser.add_argument('--reuse-cache-policy', type=str, default='reuse_if_not_modified')
    parser.add_argument('--nirvana-result-cloning-policy', type=str, default='simple')
    parser.add_argument('--clone-workflow-instance', type=str)
    parser.add_argument('--clone-to-new-workflow', action='store_true')
    parser.add_argument('--not-start', action='store_true')
    parser.add_argument('--not-keep-going', action='store_true')
    parser.add_argument('--train-fraction', type=float, default=0.7)
    parser.add_argument('--exclude-pools', default='')
    parser.add_argument('--target-priority', default='result,not_male,not_rude,target_inf_base')
    parser.add_argument('--target-weights', default='1,1,1,0.25')
    parser.add_argument('--accept-thresholds')
    parser.add_argument('--deny-thresholds')
    parser.add_argument('--pair-weight-gamma', type=str, default='0.1', help='semicolumn separated values')
    parser.add_argument('--quantization-settings', default=None, help='target_inf:target_inf_qnt10:10')
    parser.add_argument('--index-names-display-rewritten', default='base')
    parser.add_argument('--num-base-dssms', type=int, default=2, help='model and first (num-base-dssms - 1) factor dssms are base (generating candidates)')
    parser.add_argument('--lr', type=str, default='0.03:0.05:0.07:0.09:0.11', help='semicolumn separated values')
    parser.add_argument('--num-iter', type=int, default=10000)
    parser.add_argument('--config', default='prod.yaml')
    parser.add_argument('--calc-fstr', action='store_true')
    parser.add_argument('--precalced-factors', default=None)
    parser.add_argument('--curve-axis', default='result_ndcg,target_inf_indep_ndcg')
    parser.add_argument('--draw-curve', action='store_true')
    parser.add_argument('--context-length', type=int, default=3)
    parser.add_argument('--apply-dssm-to-rewritten', action='store_true')
    parser.add_argument('--external-validation', default='')
    parser.add_argument('--like-prod-validation')
    parser.add_argument('--base-model')
    parser.add_argument('--factor-model')
    parser.add_argument('--factor-model-1')
    parser.add_argument('--factor-model-2')
    parser.add_argument('--func-factors', help='comma separated function names')
    parser.add_argument('--rerank-validation', action='store_true')
    args = parser.parse_args()

    config = yaml.load(open(args.config))
    print(config)
    config['shard_config'] = yaml.load(open(config['shard_config']))

    with vh.Graph() as g, vh.NirvanaBackend(), vh.cwd('reranker'):
        train_reranker(args, config)
    keeper = run_async(g, args, label_prefix='[reranker]')
    keeper.get_workflow_info()
