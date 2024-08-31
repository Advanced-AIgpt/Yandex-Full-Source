import vh
import uuid
import hashlib
import re
import string
import os
import getpass
import yt.wrapper as yt
import itertools

get_mr_table_op = vh.op(id="6ef6b6f1-30c4-4115-b98c-1ca323b50ac0").partial(yt_table_outputs=['outTable'])
get_mr_dir_op = vh.op(id="eec37746-6363-42c6-9aa9-2bfebedeca60").partial(yt_table_outputs=['mr_directory'])
mr_merge_op = vh.op(id="66fdd3dc-23a2-11e7-ac34-3c970e24a776").partial(yt_table_outputs=['merged'])
mr_sort_op = vh.op(id="51061ea1-c630-4fa1-a2e5-53afd15db896").partial(yt_table_outputs=['sorted'])
mr_move_op = vh.op(id="83f0cf88-63d9-11e6-a050-3c970e24a776").partial(yt_table_outputs=['moved'])
mr_copy_op = vh.op(id="23762895-cf87-11e6-9372-6480993f8e34").partial(yt_table_outputs=['copy'])
yt_join_op = vh.op(id="7b4c1cd8-edac-4569-9cc3-d126d4b78a23").partial(yt_table_outputs=['table'])
mr_cat_op = vh.op(id="235f134e-74d0-4295-b7d9-a383d80d9c22")
mr_read_tsv_op = vh.op(id="ac31b077-33aa-4e74-99d6-0f8ddd8e25bb")
tsv_map_op = vh.op(id="6ebb590a-1348-4984-a85f-3911819dad7e")
mx_ops_calc_op = vh.op(id="d8468785-1c94-413e-b566-a7c485edd214")
train_catboost_op = vh.op(id="3cae0255-dbd2-4e42-8645-d52fc097488c")
cb2mxnet_op = vh.op(id="70bb66fd-00f9-490a-adc6-8b9be47bc8c7")
arcadia_export_file_op = vh.op(id="940e9ba8-11a0-4daa-ae23-8bf71f11e04c")
sky_get_op = vh.op(id="1be996b0-1ac0-4360-92c8-1bdd8710c856")
calc_factors_op = vh.op(id="e69d154b-7bf1-48a3-9a4e-c120da55760f", allow_deprecated=False).partial(yt_table_outputs=['output'])
calc_reranker_metrics_op = vh.op(id="0cbc33a4-b202-4e4c-afea-e84f771d9678", allow_deprecated=False)
apply_dssm_op = vh.op(id="ede38501-8925-423c-9bff-62e1643c2a9f", allow_deprecated=False).partial(yt_table_outputs=['output'])
train_test_split_op = vh.op(id="af8946b7-1402-4ed1-91ad-20e9ff1da85a", allow_deprecated=False).partial(yt_table_outputs=['dst'])
build_mx_pool_op = vh.op(id="82826518-36db-4c14-835d-5e7d6e5ae369", allow_deprecated=False).partial(yt_table_outputs=['output', 'output_pairs', 'output_reply_freq'])
# no neutral bad rank :
#build_mx_pool_op = vh.op(id="20d728ac-3c5b-49d0-8175-2b22f35423f9", allow_deprecated=False).partial(yt_table_outputs=['output', 'output_pairs'])
find_top_replies_op = vh.op(id="3b232fcc-e31f-4fbc-9433-c571d34b4979", allow_deprecated=False).partial(yt_table_outputs=['output_table'])
find_top_replies_hnsw_op = vh.op(id="13ea2ed1-c17e-4154-9f42-d4f54a6f1c29", allow_deprecated=False).partial(yt_table_outputs=['output'])
punctuate_replies_op = vh.op(id="c0b54b1c-d906-4c3f-9303-75701daa14e3", allow_deprecated=False).partial(yt_table_outputs=['output'])
rerank_by_mx_ops_op = vh.op(id="51907f27-bde1-4fe4-bca0-476bef9968af", allow_deprecated=False).partial(yt_table_outputs=['dst'])
coverage_metrics_op = vh.op(id="7ed5662b-6443-4fd3-83b4-6f99ba2336d3", allow_deprecated=False)
yt_cut_op = vh.op(id="e7d042cc-3579-4672-8dc3-64fbe6b6166d", allow_deprecated=False).partial(yt_table_outputs=['output'])
yt_list_op = vh.op(id="31032446-7b13-41b2-aeb2-ed9b7d2a5fa1", allow_deprecated=False)
calc_informativeness_op = vh.op(id="ffedc5a1-4d6d-41bd-a24e-ae0a574eccea", allow_deprecated=False).partial(yt_table_outputs=['output'])
draw_curve_op = vh.op(id="f8a907ad-2e82-4293-a03e-a7de9b71dbbb", allow_deprecated=False)
run_yql_op = vh.op(id="d4d37b1b-bf34-449a-b603-678f6f00e49b").partial(yt_table_outputs=['output1'])


def get_mr_table(mr_table, base_path=None):
    return get_mr_table_op(
        _inputs={
            'base_path': base_path,
        },
        _options={
            'cluster': 'hahn',
            'creationMode': 'CHECK_EXISTS',
            'table': mr_table,
            'yt-token': vh.get_yt_token_secret(),
        },
    )['outTable']


def get_mr_dir(mr_dir):
    return get_mr_dir_op(
        _options={
            'cluster': 'hahn',
            'creationMode': 'CHECK_EXISTS',
            'path': mr_dir,
            'yt-token': vh.get_yt_token_secret(),
        },
    )['mr_directory']


def mr_sort(tables, sort_by, mr_account=None):
    return mr_sort_op(
        _inputs={
            'srcs': tables,
        },
        _options={
            'sort-by': sort_by,
            'yt-token': vh.get_yt_token_secret(),
            'mr-account': vh.get_mr_account() if not mr_account else mr_account,
            'timestamp': '2018-10-03T03:47:43+0300',
            'yt-pool': vh.get_yt_pool(),
        },
    )['sorted']


def mr_merge(tables, mr_account=None):
    return mr_merge_op(
        _inputs={
            'srcs': tables,
        },
        _options={
            'merge-mode': 'AUTO',
            'spec': '{"schema_inference_mode": "from_output"}',
            'append': False,
            'yt-token': vh.get_yt_token_secret(),
            'mr-account': vh.get_mr_account() if not mr_account else mr_account,
            'timestamp': '2018-09-24T17:25:12+0300',
            'yt-pool': vh.get_yt_pool(),
        },
    )['merged']


def cache_table(table, mr_account):
    return mr_merge(table, mr_account)


def mr_move(table_in, table_out, force=False, preserve_account=False):
    return mr_move_op(
        _inputs={
            'src': table_in,
        },
        _options={
            'dst-path': table_out,
            'force': force,
            'preserve-account': preserve_account,
            'yt-token': vh.get_yt_token_secret(),
            'timestamp': '2018-09-24T17:25:12+0300',
        },
    )['moved']


def mr_copy(table_in, table_out, force=False, preserve_account=False, mr_account=None):
    return mr_copy_op(
        _inputs={
            'src': table_in,
        },
        _options={
            'dst-cluster': 'hahn',
            'dst-path': table_out,
            'force': force,
            'preserve-account': preserve_account,
            'yt-token': vh.get_yt_token_secret(),
            'mr-account': vh.get_mr_account() if not mr_account else mr_account,
            'timestamp': '2018-09-24T17:25:12+0300',
        },
    )['copy']


def mr_read_tsv(table, columns, escaping=False):
    return mr_read_tsv_op(
        _inputs={
            'table': table,
        },
        _options={
            'ttl': 360,
            'max-ram': 1024,
            'max-disk': 10240,
            'yt-token': vh.get_yt_token_secret(),
            'columns': columns,
            'missing-value-mode': 'FAIL',
            'output-escaping': escaping,
        },
    )['tsv']


def yt_cut(table, columns, mr_account=None):
    return yt_cut_op(
        _inputs={
            'input': table,
        },
        _options={
            'yt-token': vh.get_yt_token_secret(),
            'mr-account': vh.get_mr_account() if not mr_account else mr_account,
            'ttl': 360,
            'max-ram': 100,
            'columns': columns,
        },
    )['output']


def yt_join(left, right, by, type='inner', mr_account=None):
    return yt_join_op(
        _inputs={
            'left_table': left,
            'right_table': right,
        },
        _options={
            'yt-token': vh.get_yt_token_secret(),
            'mr-account': vh.get_mr_account() if not mr_account else mr_account,
            'yt-pool': vh.get_yt_pool(),
            'max-ram': 1000,
            'by': by,
            'type': type,
        },
    )['table']


def arcadia_export_file(path, revision):
    return arcadia_export_file_op(
        _options={
            'arcadia_path': path,
            'revision': revision,
        },
    )


def sky_get(rbtorrent):
    return sky_get_op(
        _options={
            'rbtorrent': rbtorrent,
            'out_type': 'binary',
            'max-disk': 100000
        },
    )['file_binary']


@vh.module(input_table=vh.YTTable, output_table=vh.mkoutput(vh.YTTable), revision=vh.mkoption(int))
def rewrite_replies(input_table, revision, output_table):
    reply_rewriter = vh.arcadia_executable('alice/boltalka/tools/reply_rewriter')
    replies_sed = arcadia_export_file('arcadia/alice/boltalka/scripts/normalize_and_filter/replies_sed.tsv', revision)['tsv']
    replies_rewritten = arcadia_export_file('arcadia/alice/boltalka/scripts/normalize_and_filter/rewritten_replies.tsv', revision)['tsv']
    vh.tgt(output_table, input_table, reply_rewriter, replies_sed, replies_rewritten,
        recipe='YT_PROXY=hahn {{ reply_rewriter }} --src {{ IN }} --dst {{ OUT }} rewrite --sed-rewrites-file {{ replies_sed }} --rewritten-replies-file {{ replies_rewritten }}',
        hardware_params=vh.HardwareParams(max_ram=10000))
    return output_table


def punctuate_replies(table):
    return punctuate_replies_op(
        _inputs={
            'input': table,
        },
        _options={
            'yt-token': vh.get_yt_token_secret(),
            'mr-account': vh.get_mr_account(),
            'job-count': 300,
        },
    )['output']


@vh.module(input_table=vh.YTTable, output_table=vh.mkoutput(vh.YTTable), revision=vh.mkoption(int), set_not_you=vh.mkoption(bool, default=False))
def add_more_respect(input_table, revision, set_not_you, output_table):
    reply_rewriter = vh.arcadia_executable('alice/boltalka/tools/reply_rewriter')
    respect_banlist = arcadia_export_file('arcadia/alice/boltalka/tools/reply_rewriter/respect_banlist.txt', revision)['binary']
    respect_model = sky_get('rbtorrent:703ca62ff37513890355f0a460d911985984cd86')
    vh.tgt(output_table, input_table, reply_rewriter, respect_model, respect_banlist,
        recipe='YT_PROXY=hahn {{ reply_rewriter }} --src {{ IN }} --dst {{ OUT }} add_more_respect --respect-catboost-model {{ respect_model }} --respect-banlist {{ respect_banlist }} {% if set_not_you %} --respect-set-not-you {% endif %}',
        hardware_params=vh.HardwareParams(max_ram=1000))
    return output_table


def train_test_split(table, split_by, split_value_base, train_fraction, is_test, mr_account=None):
    return train_test_split_op(
        _inputs={
            'src': table,
        },
        _options={
            'yt-token': vh.get_yt_token_secret(),
            'mr-account': vh.get_mr_account() if not mr_account else mr_account,
            'ttl': 360,
            'max-ram': 100,
            'split-by': split_by,
            'split-value-base': split_value_base,
            'train-fraction': train_fraction,
            'is-test': is_test,
        },
    )['dst']


class AddStrColumnMapper(object):
    def __init__(self, key, value):
        self.key = key
        self.value = value
    def __call__(self, row):
        row[self.key] = self.value
        yield row

@vh.lazy.hardware_params(vh.HardwareParams(max_ram=1000))
@vh.lazy(input_table=vh.YTTable, output_table=vh.mkoutput(vh.YTTable), key=vh.mkoption(str), value=vh.mkoption(str))
def add_str_column(input_table, key, value, output_table):
    yt.run_map(AddStrColumnMapper(key, value), input_table, output_table)
    return output_table


class SelectRowsByRangeMapper(object):
    def __init__(self, column, low=None, high=None):
        assert low is not None or high is not None
        self.column = column
        self.low = low
        self.high = high

    def __call__(self, row):
        value = row.get(self.column, 0)
        if self.high is None:
            if value >= self.low:
                yield row
        elif self.low is None:
            if value < self.high:
                yield row
        elif self.low <= value < self.high:
            yield row

@vh.lazy.hardware_params(vh.HardwareParams(max_ram=1000))
@vh.lazy(input_table=vh.YTTable, output_table=vh.mkoutput(vh.YTTable), column=vh.mkoption(str), low=vh.mkoption(int, default=-1), high=vh.mkoption(int, default=-1))
def select_rows_by_range(input_table, column, low, high, output_table):
    if low == -1:
        low = None
    if high == -1:
        high = None
    assert low is not None or hign is not None
    yt.run_map(SelectRowsByRangeMapper(column, low, high), input_table, output_table)
    return output_table


class SelectRowsByValueMapper(object):
    def __init__(self, column, value, modulo):
        self.column = column
        self.value = value
        self.modulo = modulo

    def __call__(self, row):
        value = row.get(self.column, 0)
        if self.modulo != -1:
            if isinstance(value, str):
                value = int(value, 16)
            value = value % self.modulo
        if value == self.value:
            yield row

@vh.lazy.hardware_params(vh.HardwareParams(max_ram=1000))
@vh.lazy(input_table=vh.YTTable, output_table=vh.mkoutput(vh.YTTable), column=vh.mkoption(str), value=vh.mkoption(int), modulo=vh.mkoption(int, default=-1))
def select_rows_by_value(input_table, column, value, modulo, output_table):
    yt.run_map(SelectRowsByValueMapper(column, value, modulo), input_table, output_table)
    return output_table


class SelectRowsByTextColumnMapper(object):
    def __init__(self, column, value):
        self.column = column
        self.value = value

    def __call__(self, row):
        value = row.get(self.column)
        if value == self.value:
            yield row


@vh.lazy.hardware_params(vh.HardwareParams(max_ram=1000))
@vh.lazy(input_table=vh.YTTable, output_table=vh.mkoutput(vh.YTTable), column=vh.mkoption(str), value=vh.mkoption(str))
def select_rows_by_text_column(input_table, column, value, output_table):
    yt.run_map(SelectRowsByTextColumnMapper(column, value), input_table, output_table)
    return output_table


def add_query_id_mapper(row):
    if not row.get("query_id"):
        key = []
        if "context_id" in row:
            key.append(str(row["context_id"]))
            del row["context_id"]
        for i in itertools.count():
            k = "query_{}".format(i)
            if k not in row:
                break
            key.append(row[k])
        key_str = key[0] + "\t".join(key[1:])
        row["query_id"] = hashlib.md5(key_str).hexdigest()
    yield row

@vh.lazy.hardware_params(vh.HardwareParams(max_ram=1000))
@vh.lazy(input_table=vh.YTTable, output_table=vh.mkoutput(vh.YTTable))
def add_query_id(input_table, output_table):
    yt.run_map(add_query_id_mapper, input_table, output_table)
    return output_table


def normalize_text(text):
    s = unicode(text, "utf-8")
    s = re.sub(r"([" + string.punctuation + r"\\])", r" \1 ", s).strip()
    s = re.sub(r"\s+", " ", s, flags=re.U).strip()
    return s.lower()


class NormalizeTextMapper(object):
    def __init__(self, columns):
        self.columns = columns

    def __call__(self, row):
        for column in self.columns:
            row[column] = normalize_text(row[column])
        yield row


@vh.lazy.hardware_params(vh.HardwareParams(max_ram=1000))
@vh.lazy(input_table=vh.YTTable, output_table=vh.mkoutput(vh.YTTable), columns=vh.mkoption(str, nargs='+'))
def normalize_columns(input_table, columns, output_table):
    yt.run_map(NormalizeTextMapper(columns), input_table, output_table)
    return output_table


class DeleteColumnsMapper(object):
    def __init__(self, columns):
        self.columns = columns

    def __call__(self, row):
        for col in self.columns:
            del row[col]
        yield row


@vh.lazy.hardware_params(vh.HardwareParams(max_ram=1000))
@vh.lazy(input_table=vh.YTTable, output_table=vh.mkoutput(vh.YTTable), columns=vh.mkoption(str, nargs='+'))
def delete_columns(input_table, columns, output_table):
    yt.run_map(DeleteColumnsMapper(columns), input_table, output_table)
    return output_table


class CopyColumnMapper(object):
    def __init__(self, column_from, column_to):
        self.column_from = column_from
        self.column_to = column_to

    def __call__(self, row):
        row[self.column_to] = row[self.column_from]
        yield row


@vh.lazy.hardware_params(vh.HardwareParams(max_ram=1000))
@vh.lazy(input_table=vh.YTTable, output_table=vh.mkoutput(vh.YTTable), column_from=vh.mkoption(str), column_to=vh.mkoption(str))
def copy_column(input_table, column_from, column_to, output_table):
    yt.run_map(CopyColumnMapper(column_from, column_to), input_table, output_table)
    return output_table


class SelectTopReducer(object):
    def __init__(self, top_size, unique_by):
        self.top_size = top_size
        self.unique_by = unique_by

    def __call__(self, key, rows):
        unique_values = set()
        num_rows = 0
        for row in rows:
            if self.top_size is not None and num_rows >= self.top_size:
                return
            if self.unique_by and row[self.unique_by] in unique_values:
                continue
            yield row
            if self.unique_by:
                unique_values.add(row[self.unique_by])
            num_rows += 1

@vh.lazy.hardware_params(vh.HardwareParams(max_ram=1000))
@vh.lazy(input_table=vh.YTTable, output_table=vh.mkoutput(vh.YTTable), reduce_by=vh.mkoption(str, nargs='+'), sort_by=vh.mkoption(str, nargs='*'),
         top_size=vh.mkoption(int), unique_by=vh.mkoption(str, default=''))
def select_querywise_top(input_table, reduce_by, sort_by, top_size, unique_by, output_table):
    if not sort_by:
        sort_by = reduce_by
    if not yt.get(input_table + '/@sorted') or yt.get(input_table + '/@sorted_by')[:len(sort_by)] != sort_by:
        yt.run_sort(input_table, output_table, sort_by=sort_by)
        input_table = output_table
    yt.run_reduce(SelectTopReducer(top_size, unique_by), input_table, output_table, reduce_by=reduce_by, sort_by=sort_by)
    return output_table


def remove_duplicates(table, reduce_by, sort_by=[]):
    return select_querywise_top(table, reduce_by=reduce_by, sort_by=sort_by, top_size=1)


def calc_frequency_reducer(key, rows):
    assert 'inv_freq' not in key
    res = dict(key)
    res['inv_freq'] = -sum(1 for _ in rows)
    yield res

@vh.lazy.hardware_params(vh.HardwareParams(max_ram=1000))
@vh.lazy(input_table=vh.YTTable, output_table=vh.mkoutput(vh.YTTable), reduce_by=vh.mkoption(str, nargs='+'))
def calc_frequency(input_table, reduce_by, output_table):
    if not yt.get(input_table + '/@sorted') or yt.get(input_table + '/@sorted_by')[:len(reduce_by)] != reduce_by:
        yt.run_sort(input_table, output_table, sort_by=reduce_by)
        input_table = output_table
    yt.run_reduce(calc_frequency_reducer, input_table, output_table, reduce_by=reduce_by)
    return output_table


def calc_coverage_metrics_from_freq_table(freq_table, column, top_sizes, skip_punctuation, ngram, eos, mr_account=None):
    return coverage_metrics_op(
        _inputs={
            'src': freq_table,
        },
        _options={
            'yt-token': vh.get_yt_token_secret(),
            'mr-account': vh.get_mr_account() if not mr_account else mr_account,
            'ttl': 360,
            'max-ram': 100,
            'column': column,
            'top-sizes': ','.join(map(str, top_sizes)),
            'skip-punctuation': skip_punctuation,
            'ngram': ngram,
            'eos': eos,
        }
    )


def calc_coverage_metrics(sorted_tops, column, top_size):
    table = select_querywise_top(sorted_tops, reduce_by='query_id', sort_by=['query_id', 'inv_reranker_score'], top_size=top_size, unique_by=column)
    table = calc_frequency(table, reduce_by=[column])
    table = mr_sort(table, ['inv_freq'])
    return calc_coverage_metrics_from_freq_table(table, column=column, top_sizes=[10, 100, 1000, 10000], skip_punctuation=False, ngram=None, eos=False)['report']


def calc_reranker_metrics(table, training_log=None, model_name=None, rerank_by_targets=None, no_rerank=False, inverse=False, top='1', unique_top=False, mr_account=None):
    return calc_reranker_metrics_op(
        _inputs={
            'src': table,
            'training_log': training_log,
        },
        _options={
            'model-name': model_name,
            'rerank-by-targets': rerank_by_targets,
            'no-rerank': no_rerank,
            'inverse': inverse,
            'top': top,
            'unique-top': unique_top,
            'yt-token': vh.get_yt_token_secret(),
            'mr-account': vh.get_mr_account() if not mr_account else mr_account,
            'ttl': 360,
            'max-ram': 500,
        },
    )


def calc_factors(table, dssm_models, rus_lister, num_base_dssms, lang='ru', mr_account=None):
    return calc_factors_op(
        _inputs={
            'input': table,
            'ruslister_map': rus_lister,
            'model': dssm_models['base_model'],
            'factor_dssm_0': dssm_models.get('factor_model'),
            'factor_dssm_1': dssm_models.get('factor_model_1'),
            'factor_dssm_2': dssm_models.get('factor_model_2'),
        },
        _options={
            'yt-token': vh.get_yt_token_secret(),
            'mr-account': vh.get_mr_account() if not mr_account else mr_account,
            'yt-pool': vh.get_yt_pool(),
            'ttl': 360,
            'max-ram': 500,
            'num-base-dssm-indexes': num_base_dssms,
            'lang': lang,
        },
    )['output']


def train_catboost(catboost_cd, train_pool, train_pairs, validation_pool, validation_pairs, lr, num_iter, calc_fstr=True):
    return train_catboost_op(
        _name='lr=%.10g,iter=%d' % (lr, num_iter),
        _inputs={
            'cd': catboost_cd,
            'learn': train_pool,
            'pairs': train_pairs,
            'test': validation_pool,
            'test_pairs': validation_pairs,
        },
        _options={
            'ttl': 3600,
            'gpu-type': 'CUDA_6_1',
            'cpu-guarantee': 1600,
            'loss-function': 'PairLogitPairwise',
            'iterations': num_iter,
            'learning-rate': lr,
            'seed': 0,
            'create-tensorboard': True,
            'use-best-model': True,
            'one-hot-max-size': 128,
            'has-header': False,
            'args': '-x128  --bootstrap-type Bernoulli --subsample 0.5 --leaf-estimation-method Newton --l2-leaf-reg 5.0  --leaf-estimation-iterations 5 --feature-border-type UniformAndQuantiles',
            'yt-token': vh.get_yt_token_secret(),
            'yt_pool': vh.get_yt_pool(),
            'fstr-type': 'PredictionValuesChange' if not calc_fstr else None,
        },
    )


@vh.lazy.hardware_params(vh.HardwareParams(max_ram=1000))
@vh.lazy(input_table=vh.YTTable, output_file=vh.mkoutput(vh.File))
def get_catboost_column_description(input_table, output_file):
    row = next(yt.read_table(input_table))
    assert 'value' in row
    num_features = len(row['value'].split('\t'))
    with open(output_file, 'wt') as f:
        f.write('0\tQueryId\n')
        f.write('1\tLabel\n')
        f.write('2\tAuxiliary\n')
        f.write('3\tSubgroupId\n')
        for i in range(4, num_features):
            f.write('%d\tNum\n' % i)
    return output_file


def convert_cb2mxnet(model):
    return cb2mxnet_op(
        _inputs={
            'catboost.cbm': model,
        },
        _options={
            'offset': 0,
            'max-ram': 100,
            'max-disk': 1024,
        },
    )['matrixnet.info']


def mx_ops_calc(model, pool):
    return mx_ops_calc_op(
        _inputs={
            'model.info': model,
            'pool': pool,
        },
        _options={
            'ttl': 360,
            'max-ram': 2048,
            'cpu-cores': 1,
            'job-is-vanilla': False,
            'skip': 4,
            'yt-token': vh.get_yt_token_secret(),
            'max-disk': 2048,
            'mr-destination-table': os.path.join('//tmp', getpass.getuser(), uuid.uuid4().hex),
        },
    )['mr_result']


def preprocess_mx_pairs_mapper(row):
    if row['weight'] == 0:
        return
    res = {}
    res['key'] = str(row['pos_id'])
    res['value'] = str(row['neg_id']) + '\t' + str(row['weight'])
    yield res

@vh.lazy.hardware_params(vh.HardwareParams(max_ram=1000))
@vh.lazy(input_table=vh.YTTable, output_table=vh.mkoutput(vh.YTTable))
def preprocess_mx_pairs(input_table, output_table):
    yt.run_map(preprocess_mx_pairs_mapper, input_table, output_table)
    return output_table


def build_mx_pool(table, priority=None, priority_weights=None, accept_thresholds=None, deny_thresholds=None, mr_account=None):
    op = build_mx_pool_op(
        _inputs={
            'input': table,
        },
        _options={
            'yt-token': vh.get_yt_token_secret(),
            'mr-account': vh.get_mr_account() if not mr_account else mr_account,
            'yt-pool': vh.get_yt_pool(),
            'ttl': 360,
            'max-ram': 500,
            'priority': priority,
            'mode': 'pairwise',
            'train-fraction': 1,
            'is-test': False,
            'normalize-by-num-pairs': 'none',
            'priority-weights': priority_weights,
            'accept-thresholds': accept_thresholds,
            'deny-thresholds': deny_thresholds,
            'use-priority-weights-for-pair-count': False,
        },
    )
    return op['output'], op['output_pairs'], op['output_reply_freq']


def apply_dssm(table, dssm_models, context_column_prefix='context_', reply_column='reply', lang='ru', mr_account=None):
    assert dssm_models
    return apply_dssm_op(
        _inputs={
            'input': table,
            'model': dssm_models.get('base_model'),
            'factor_dssm_0': dssm_models.get('factor_model'),
            'factor_dssm_1': dssm_models.get('factor_model_1'),
            'factor_dssm_2': dssm_models.get('factor_model_2'),
        },
        _options={
            'yt-token': vh.get_yt_token_secret(),
            'mr-account': vh.get_mr_account() if not mr_account else mr_account,
            'yt-pool': vh.get_yt_pool(),
            'ttl': 360,
            'max-ram': 1000,
            'job-count': 300,
            'batch-size': 100,
            'lang': lang,
            'context-column-prefix': context_column_prefix,
            'reply-column': reply_column,
        },
    )['output']


class PostprocessAfterTopFindingMapper(object):
    def __init__(self, dssm_id):
        self.dssm_id = dssm_id

    def __call__(self, row):
        row["dssm_id"] = self.dssm_id
        context = unicode(row["context"], "utf-8")
        for i, turn in enumerate(context.split("\t")):
            row["query_" + str(2 - i)] = turn
        reply_context = unicode(row["reply_context"], "utf-8")
        for i, turn in enumerate(reply_context.split("\t")):
            row["context_" + str(2 - i)] = turn
        row["query_id"] = hashlib.md5(str(row["context_id"]) + "\t" + row["context"]).hexdigest()
        yield row

@vh.lazy(input_table=vh.YTTable, output_table=vh.mkoutput(vh.YTTable), dssm_id=vh.mkoption(int))
def postprocess_after_top_finding(input_table, dssm_id, output_table):
    yt.run_map(PostprocessAfterTopFindingMapper(dssm_id), input_table, output_table)


def find_top_replies_exact(table, index, top_size, context_weight=0.5, num_shards=1, dssm_weights=[1], mr_account=None):
    return find_top_replies_op(
        _inputs={
            'contexts': table,
            'replies': index
        },
        _options={
            'yt-token': vh.get_yt_token_secret(),
            'mr-account': vh.get_mr_account() if not mr_account else mr_account,
            'yt-pool': vh.get_yt_pool(),
            'ttl': 360,
            'max-ram': 1000,
            'num-shards': num_shards,
            'context-weight': context_weight,
            'top': top_size,
            'thread-count': 8,
            'factor-weights': ','.join(map(str, dssm_weights)),
        },
    )['output_table']


def find_top_replies_hnsw(table, vectors, ids, index, top_size,
                          search_neighborhood_size=500, distance_calc_limit=8500, prefix="",
                          context_weight=0.5, mr_account=None):
    return find_top_replies_hnsw_op(
        _inputs={
            'input': table,
            'hnsw_index': index,
            'hnsw_vectors': vectors,
            'hnsw_ids': ids
        },
        _options={
            'yt-token': vh.get_yt_token_secret(),
            'mr-account': vh.get_mr_account() if not mr_account else mr_account,
            'yt-pool': vh.get_yt_pool(),
            'ttl': 360,
            'max-ram': 100,
            'context-weight': context_weight,
            'dim': 300,
            'top-size': top_size,
            'search-neighborhood-size': search_neighborhood_size,
            'distance-calc-limit': distance_calc_limit,
            'embedding-column-prefix': prefix,
        },
    )['output']


def rerank(table, mx_model, mx_pool=None):
    if mx_pool is None:
        mx_pool = build_mx_pool(table)[0]
    mx_ops = mx_ops_calc(mx_model, mx_pool)
    return rerank_by_mx_ops_op(
        _inputs={
            'src': table,
            'mx_ops': mx_ops,
        },
        _options={
            'yt-token': vh.get_yt_token_secret(),
            'mr-account': vh.get_mr_account(),
            'yt-pool': vh.get_yt_pool(),
            'ttl': 360,
            'max-ram': 1000,
            'inv-score-column': 'inv_reranker_score',
        },
    )['dst'], mx_ops


def calc_informativeness(table, negative_contexts, informativeness_column=None, denominator_column=None,
                         reply_embedding_column=None, context_embedding_column=None, negative_embedding_column=None):
    return calc_informativeness_op(
        _inputs={
            'input': table,
            'negative_contexts': negative_contexts,
        },
        _options={
            'yt-token': vh.get_yt_token_secret(),
            'mr-account': vh.get_mr_account(),
            'yt-pool': vh.get_yt_pool(),
            'ttl': 360,
            'max-ram': 1000,
            'informativeness-column': informativeness_column,
            'denominator-column': denominator_column,
            'reply-embedding-column': reply_embedding_column,
            'context-embedding-column': context_embedding_column,
            'negative-embedding-column': negative_embedding_column,
        },
    )['output']


def draw_curve(input, x_name, y_name, baseline=None):
    return draw_curve_op(
        _inputs={
            'input': input,
            'baseline': baseline,
        },
        _options={
            'x-name': x_name,
            'y-name': y_name,
        })['output']


def run_yql(table, request, mr_account=None):
    return run_yql_op(
        _inputs={
            'input1': table,
        },
        _options={
            'yt-token': vh.get_yt_token_secret(),
            'mr-account': vh.get_mr_account() if not mr_account else mr_account,
            'yql-token': 'boltalka_yql_token',
            'request': request,
        },
    )['output1']
