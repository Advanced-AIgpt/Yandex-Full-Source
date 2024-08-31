import vh
import yt.wrapper as yt
import numpy as np
from collections import defaultdict
import itertools
import inspect
import bisect


INFORMATIVENESS_DENOMINATOR_STATIC_FACTOR_IDX = 8
INFORMATIVENESS_DENOMINATOR_STATIC_FACTOR_COLUMN = 'static_factor_{}'.format(INFORMATIVENESS_DENOMINATOR_STATIC_FACTOR_IDX)
INFORMATIVENESS_DYNAMIC_FACTOR_IDS_DCT = {'ru': 536, 'tr': 258}
SELECTIVE_MODEL_NAME = 'selective'
SEQ2SEQ_MODEL_NAME = 's2s'
NUM_COSINE_FACTORS = 4
DSSM_CONTEXT_REPLY_COSINE_DYNAMIC_FACTOR_IDS = {
    'base_model': 0,
    'factor_model': 531,
    'factor_model_1': 535,
}
DSSM_QUERY_REPLY_COSINE_DYNAMIC_FACTOR_IDS = {
    'base_model': 2,
    'factor_model': 533,
    'factor_model_1': 537,
}
DSSM_COSINE_DYNAMIC_FACTOR_IDS = {
    'base_model': range(DSSM_CONTEXT_REPLY_COSINE_DYNAMIC_FACTOR_IDS['base_model'], DSSM_CONTEXT_REPLY_COSINE_DYNAMIC_FACTOR_IDS['base_model'] + NUM_COSINE_FACTORS),
    'factor_model': range(DSSM_CONTEXT_REPLY_COSINE_DYNAMIC_FACTOR_IDS['factor_model'], DSSM_CONTEXT_REPLY_COSINE_DYNAMIC_FACTOR_IDS['factor_model'] + NUM_COSINE_FACTORS),
    'factor_model_1': range(DSSM_CONTEXT_REPLY_COSINE_DYNAMIC_FACTOR_IDS['factor_model_1'], DSSM_CONTEXT_REPLY_COSINE_DYNAMIC_FACTOR_IDS['factor_model_1'] + NUM_COSINE_FACTORS),
}
EMBEDDING_PREFIXES = {
    'base_model': '',
    'factor_model': 'factor_dssm_0:',
    'factor_model_1': 'factor_dssm_1:',
}
INFORMATIVENESS_MODEL = 'factor_model_1'


def calc_num_factors(table_name, prefix=''):
    row = next(yt.read_table(table_name))
    i = 0
    while prefix + 'factor_' + str(i) in row:
        i += 1
    return i


class ReweighPairsByFreqMapper(object):
    def __init__(self, gamma):
        self.gamma = gamma
    def __call__(self, row):
        count = row.get('cnt_pos_pairs', 0)
        row['weight'] /= (count + 1.0) ** self.gamma
        yield row

@vh.lazy.hardware_params(vh.HardwareParams(max_ram=1000))
@vh.lazy(input_table=vh.YTTable, output_table=vh.mkoutput(vh.YTTable), gamma=vh.mkoption(float))
def reweigh_pairs_by_freq(input_table, gamma, output_table):
    yt.run_map(ReweighPairsByFreqMapper(gamma), input_table, output_table)
    return output_table


def convert_from_binary(emb_str):
    return np.fromstring(emb_str, dtype=np.float32)


def read_embeddings(tablename, column):
    embeddings = []
    for row in yt.read_table(tablename):
        embeddings.append(convert_from_binary(row[column]))
    return np.array(embeddings)


def calc_informativeness_denominator(reply_embedding_bin, negative_context_embeddings, dot_product_scale):
    return np.sum(np.exp(dot_product_scale * np.dot(negative_context_embeddings, convert_from_binary(reply_embedding_bin)))).item()


def calc_informativeness_score(dssm_cosine, dot_product_scale, denom, numer_in_denom=False):
    numer = np.exp(dot_product_scale * dssm_cosine)
    if numer_in_denom:
        denom += numer
    score = numer / denom * 1000
    return score


class EditFeaturesMapper(object):
    def __init__(self, informativeness_factor_idx, num_factors, add_s2s_features,
                 dssm_query_reply_cosine_factor_idx, dot_product_scale):
        self.informativeness_factor_idx = informativeness_factor_idx
        self.num_factors = num_factors
        self.add_s2s_features = add_s2s_features
        self.dssm_query_reply_cosine_factor_name = self._factor_name(dssm_query_reply_cosine_factor_idx)
        self.dot_product_scale = dot_product_scale
        self.reply_embedding_column = EMBEDDING_PREFIXES[INFORMATIVENESS_MODEL] + 'reply_embedding'

    def _factor_name(self, idx):
        return 'factor_' + str(idx)

    def _append_factors(self, row, new_factors):
        for i in range(len(new_factors)):
            row[self._factor_name(self.num_factors + i)] = new_factors[i]

    def _set_value(self, row, factor_ids, value):
        for idx in factor_ids:
            row[self._factor_name(idx)] = value

    def __call__(self, row):
        if 'is_assessor_index' in row:
            row['target_ass'] = row['is_assessor_index']
        row['target_interest'] = int(row['mark'] == 'interesting') if (row.get('mark') is not None) else None
        is_s2s_candidate = row['model_type'] == SEQ2SEQ_MODEL_NAME
        row['target_s2s'] = int(is_s2s_candidate)
        row['target_inf_base'] = row[self._factor_name(self.informativeness_factor_idx)]
        if is_s2s_candidate:
            self._set_value(row, [INFORMATIVENESS_DENOMINATOR_STATIC_FACTOR_IDX, self.informativeness_factor_idx], 0.)
        new_factors = []
        if self.add_s2s_features:
            new_factors.append(int(is_s2s_candidate))
            new_factors.append(row['s2s_score'] if is_s2s_candidate else 0.)
            #new_factors.append(row['s2s_num_tokens'] if is_s2s_candidate else 0)
        self._append_factors(row, new_factors)
        yield row


@vh.lazy.hardware_params(vh.HardwareParams(max_ram=1000))
@vh.lazy(input_table=vh.YTTable, output_table=vh.mkoutput(vh.YTTable), lang=vh.mkoption(str),
         add_s2s_features=vh.mkoption(bool, default=True),
         dot_product_scale=vh.mkoption(float, default=1.))
def edit_features(input_table, lang, add_s2s_features, dot_product_scale, output_table):
    num_static_factors = calc_num_factors(input_table, prefix='static_')
    informativeness_factor_idx = num_static_factors + INFORMATIVENESS_DYNAMIC_FACTOR_IDS_DCT[lang]
    num_factors = calc_num_factors(input_table)
    dssm_query_reply_cosine_factor_idx = num_static_factors + DSSM_QUERY_REPLY_COSINE_DYNAMIC_FACTOR_IDS[INFORMATIVENESS_MODEL]
    yt.run_map(EditFeaturesMapper(informativeness_factor_idx, num_factors, add_s2s_features, dssm_query_reply_cosine_factor_idx,
                                  dot_product_scale), input_table, output_table)
    return output_table


class SetContextLengthMapper(object):
    def __init__(self, context_length):
        self.context_length = context_length
    def __call__(self, row):
        for i in range(self.context_length):
            for col in ['query_' + str(i), 'context_' + str(i)]:
                if col not in row:
                    row[col] = ''
        yield row

@vh.lazy.hardware_params(vh.HardwareParams(max_ram=1000))
@vh.lazy(input_table=vh.YTTable, output_table=vh.mkoutput(vh.YTTable), context_length=vh.mkoption(int))
def set_context_length(input_table, context_length, output_table):
    yt.run_map(SetContextLengthMapper(context_length), input_table, output_table)
    return output_table


class Quantizer(object):
    def __init__(self, num_bins, values):
        self.num_bins = num_bins
        sorted_values = sorted(values)
        bin_size = int(len(values) / float(num_bins))
        self.bin_borders = [sorted_values[i * bin_size] for i in range(1, num_bins)]
    def __call__(self, value):
        return bisect.bisect(self.bin_borders, value)


@vh.lazy.hardware_params(vh.HardwareParams(max_ram=1000))
@vh.lazy(object, input_table=vh.YTTable, column2num_bins=object)
def build_quantizers(input_table, column2num_bins):
    column2values = defaultdict(list)
    columns = list(column2num_bins)
    for row in yt.read_table(yt.TablePath(input_table, columns=columns)):
        for col in columns:
            column2values[col].append(row[col])
    return {col: Quantizer(column2num_bins[col], column2values[col]) for col in column2values}


class QuantizeMapper(object):
    def __init__(self, quantizers, quant_columns):
        self.quantizers = quantizers
        self.quant_columns = quant_columns
    def __call__(self, row):
        for col_from, col_to in self.quant_columns.items():
            row[col_to] = self.quantizers[col_from](row[col_from])
        yield row

@vh.lazy.hardware_params(vh.HardwareParams(max_ram=1000))
@vh.lazy(input_table=vh.YTTable, output_table=vh.mkoutput(vh.YTTable),
         quantizers=object, quant_columns=object)
def quantize(input_table, quantizers, quant_columns, output_table):
    yt.run_map(QuantizeMapper(quantizers, quant_columns), input_table, output_table)
    return output_table


def quantize_train_and_validation(train_pool, validation_pool, quantization_settings):
    column2num_bins = {}
    quant_columns = {}
    for triplet in quantization_settings.split(','):
        col_from, col_to, num_bins_str = triplet.split(':')
        column2num_bins[col_from] = int(num_bins_str)
        assert column2num_bins[col_from] > 1
        quant_columns[col_from] = col_to
    quantizers = build_quantizers(train_pool, column2num_bins)
    train_pool = quantize(train_pool, quantizers, quant_columns)
    validation_pool = quantize(validation_pool, quantizers, quant_columns)
    return train_pool, validation_pool

class FactorMapper:
    def __init__(self, func_str):
        self.func_str = func_str
        self.k = None

    def start(self):
        with open('factor_func.py', 'w') as f:
            f.write('def f(row):\n')
            f.write(self.func_str.split('\n', 1)[1])
        from factor_func import f
        self.f = f

    def __call__(self, row):
        factor = self.f(row)
        if self.k is None:
            for i in itertools.count():
                self.k = 'factor_{}'.format(i)
                if self.k not in row:
                    break
        row[self.k] = factor
        yield row


@vh.lazy.hardware_params(vh.HardwareParams(max_ram=1000))
@vh.lazy(input_table=vh.YTTable, output_table=vh.mkoutput(vh.YTTable), func_str=str)
def add_func_factor_lazy(input_table, func_str, output_table):
    yt.run_map(FactorMapper(func_str), input_table, output_table)
    return output_table


def add_func_factors(table, func_names):
    for f in func_names:
        table = add_func_factor_lazy(table, inspect.getsource(globals()[f]))
    return table


def is_entity(row):
    return int(row.get('index_name') == 'movie')
