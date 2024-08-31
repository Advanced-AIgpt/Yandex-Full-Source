from typing import Optional

import vh
from alice.boltalka.generative.pipelines.util import cast_to_mrtable
from alice.boltalka.generative.training.data.nn.util.experimental.ops import map_fn_lazy
from dict.mt.make.libs.block import block
from dict.mt.make.libs.common import MTFS, MTFSUpdate
from dict.mt.make.libs.types import MTPath
from boltalka.generative.tfnn.bucket_maker.vh import generate_bucket_op, score_bucket_op


class HideContextsMapper:
    def __init__(self, n_contexts):
        self.n_contexts = n_contexts

    def __call__(self, row):
        for i in range(self.n_contexts, 9):
            column = 'context_{}'.format(i)
            row['_hidden_' + column] = row[column]
            del row[column]
        yield row


class PutContextsBackMapper:
    def __call__(self, row):
        for i in range(9):
            column = 'context_{}'.format(i)
            hidden_column = '_hidden_' + column
            if hidden_column in row:
                row[column] = row[hidden_column]
                del row[hidden_column]

        yield row


@block
class GenerateBucketPipeline:
    input_table_path: MTPath
    output_table_path: MTPath
    model_bundle_path: MTPath
    sampling_temperature: float
    num_hypos: int
    sampling_topk: int
    unique_hypotheses_per_input: bool = False
    process_n_first_rows: int = -1
    batch_size: int = 100
    use_n_contexts: Optional[int] = None

    @staticmethod
    def configure(config):
        return GenerateBucketPipeline(
            input_table_path=config.generic.bucket_input_table,
            output_table_path=config.assessment.bucket_output_table,
        )

    def __call__(self, mtdata: MTFS) -> MTFSUpdate:
        input_table = mtdata.get_table(self.input_table_path)
        model_bundle = mtdata[self.model_bundle_path]
        out_table = vh.YTTable('tmp.yttable')

        if self.use_n_contexts is not None:
            input_table = map_fn_lazy(input_table, HideContextsMapper(self.use_n_contexts))

        with vh.HardwareParams(max_disk=10000, max_ram=16384, gpu_count=1, gpu_type=vh.GPUType.CUDA_7_0):  # TODO
            generate_bucket_op(
                input_table,
                model_bundle,
                out_table,
                self.sampling_temperature,
                self.sampling_topk,
                self.num_hypos,
                self.unique_hypotheses_per_input,
                self.process_n_first_rows,
                self.batch_size
            )

        if self.use_n_contexts is not None:
            out_table = cast_to_mrtable(map_fn_lazy(out_table, PutContextsBackMapper()))

        return {
            self.output_table_path: out_table
        }


@block
class ScoreBucketPipeline:
    input_table_path: MTPath
    output_table_path: MTPath
    model_bundle_path: MTPath
    sampling_temperature: float
    reply_column: str
    context_columns_prefix: str
    tokenize_reply_column: bool
    process_n_first_rows: int = -1

    def __call__(self, mtdata: MTFS) -> MTFSUpdate:
        input_table = mtdata.get_table(self.input_table_path)
        model_bundle = mtdata[self.model_bundle_path]
        out_table = vh.YTTable('tmp.yttable')
        score_bucket_op(
            input_table,
            model_bundle,
            out_table,
            self.sampling_temperature,
            self.reply_column,
            self.tokenize_reply_column,
            self.context_columns_prefix,
            self.process_n_first_rows
        )
        return {
            self.output_table_path: out_table
        }
