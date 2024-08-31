import re
import string

import vh
from dict.mt.make.libs.block import block
from dict.mt.make.libs.common import MTFS, MTFSUpdate
from dict.mt.make.libs.types import MTPath

import nn_applier
import yt.wrapper as yt


def extract_contexts(row):
    contexts = []
    i = 0
    while True:
        current_context = 'context_{}'.format(i)
        if current_context not in row:
            break
        contexts.append(row[current_context])

        i += 1
    return list(reversed(contexts))


def remove_trailing_none(list):
    result = []
    found_not_none = False
    for item in list:
        if found_not_none:
            result.append(item)
        else:
            if item is not None:
                found_not_none = True
                result.append(item)

    return result


def preprocess_contexts(contexts):
    # remove trailing None from contexts
    contexts = remove_trailing_none(contexts)

    # removing \n and \t from contexts
    contexts = [ctx.replace('\n', '').replace('\t', '') for ctx in contexts]

    # '-' -> ' '
    contexts = [ctx.replace('-', ' ') for ctx in contexts]

    # removing <speaker_voice='aa'>
    contexts = [re.sub('<.*>', ' ', ctx) for ctx in contexts]

    # space out punctuations
    for c in string.punctuation:
        contexts = [ctx.replace(c, ' {} '.format(c)) for ctx in contexts]

    # filtering empty contexts
    contexts = [ctx for ctx in contexts if len(ctx) > 0]

    # lowering
    contexts = [ctx.lower() for ctx in contexts]

    return contexts


def calc_dssm_scores(rows, dssm_model):
    scores = []
    for row in rows:
        contexts = preprocess_contexts(extract_contexts(row))
        preprocessed = preprocess_contexts([row['reply']])
        reply = preprocessed[0] if len(preprocessed) > 0 else ''

        scores.append(dssm_model.predict({
            'context_2': contexts[0],
            'context_1': contexts[1] if len(contexts) > 1 else '',
            'context_0': contexts[2] if len(contexts) > 2 else '',
            'reply': reply
        },
            ['output']
        )[0])
    return scores


# TODO Have to use GPU here in order to build lazy with -DTENSORFLOW_WITH_CUDA=1 flag, FIX IT!
# you can fix it with @vh.lazy.arcadia_executable_path('target/to/dummy/executable/yamake/which/includes/this/lazy/function')
@vh.lazy.hardware_params(vh.HardwareParams(max_disk=10000, max_ram=16384, gpu_count=1, gpu_type=vh.GPUType.CUDA_7_0))
@vh.lazy.from_annotations
def calc_dssm_lazy(
        bucket: vh.YTTable,
        applicable_dssm_model: vh.File,
        output_table: vh.mkoutput(vh.YTTable)
):
    dssm_model = nn_applier.Model(str(applicable_dssm_model))
    yt.config.set_proxy('hahn')  # TODO
    rows = list(yt.read_table(str(bucket)))
    scores = calc_dssm_scores(rows, dssm_model)

    assert len(rows) == len(scores)

    for row, score in zip(rows, scores):
        row['factor_dssm_score'] = score

    yt.write_table(yt.TablePath(str(output_table)), rows)
    return output_table


def calc_factor_dssm(input_table):
    dssm_model = vh.data(id='41721d0e-3501-48f9-9b39-77f216332cd2')

    out_table = vh.YTTable('tmp.yttable')

    with vh.HardwareParams(max_disk=10000, max_ram=16384, gpu_count=1, gpu_type=vh.GPUType.CUDA_7_0):  # TODO
        out_table = calc_dssm_lazy(
            input_table,
            dssm_model,
            out_table
        )

    return out_table


@block
class CalcFactorDssmPipeline:
    input_table_path: MTPath
    output_table_path: MTPath

    def __call__(self, mtdata: MTFS) -> MTFSUpdate:
        table = mtdata.get_table(self.input_table_path)
        out_table = calc_factor_dssm(table)

        return {
            self.output_table_path: out_table
        }
