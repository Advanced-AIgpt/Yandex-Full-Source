import json
import os
import tarfile

import vh
from alice.boltalka.generative.tfnn.bucket_maker.lib import generate_bucket, score_bucket

VH_DUMMY_EXECUTABLE_PATH = 'alice/boltalka/generative/tfnn/bucket_maker/vh/exec'
TENSORFLOW_GPU_FLAGS = '-DTENSORFLOW_WITH_CUDA=1'
DEFAULT_GPU_HARDWARE = vh.HardwareParams(max_disk=30000, max_ram=16384, gpu_count=1, gpu_type=vh.GPUType.CUDA_7_0)


def generate_bucket_op(
        contexts_table: vh.YTTable,
        model_archive: vh.File,
        output_table: vh.mkoutput(vh.YTTable),
        sampling_temperature=0.6,
        sampling_topk=50,
        num_hypos=1,
        unique_hypotheses_per_input=False,
        process_n_first_rows=-1,
        contexts_columns=[],
        separator_token='[SPECIAL_SEPARATOR_TOKEN]',
        batch_size=100,
        max_input_len=None,
        max_output_len=None,
        use_mapper=False,
        mapper_jobs=-1,
        yt_pool='nirvana-dialogs'
):
    return generate_bucket_op_lazy(
        contexts_table,
        model_archive,
        output_table,
        sampling_temperature,
        sampling_topk,
        num_hypos,
        unique_hypotheses_per_input,
        process_n_first_rows,
        contexts_columns,
        separator_token,
        batch_size,
        max_input_len,
        max_output_len,
        use_mapper,
        mapper_jobs,
        yt_pool
    )


def unpack_model_with_hp(archive_path, out_folder):
    with tarfile.open(str(archive_path), 'r') as tar:
        tar.extractall(out_folder)

    with open(os.path.join(out_folder, 'hp.json')) as f:
        hp = json.load(f)

    return hp


@vh.lazy.arcadia_executable_path(VH_DUMMY_EXECUTABLE_PATH, build_flags=TENSORFLOW_GPU_FLAGS)
@vh.lazy.hardware_params(DEFAULT_GPU_HARDWARE)
@vh.lazy.from_annotations
def generate_bucket_op_lazy(
        contexts_table: vh.YTTable,
        model_archive: vh.File,
        output_table: vh.mkoutput(vh.YTTable),
        sampling_temperature: vh.mkoption(float, default=0.6),
        sampling_topk: vh.mkoption(int, default=50),
        num_hypos: vh.mkoption(int, default=1),
        unique_hypotheses_per_input: vh.mkoption(bool, default=False),
        process_n_first_rows: vh.mkoption(int, default=-1),
        contexts_columns: vh.mkoption(str, nargs='+'),
        separator_token: vh.mkoption(str, default='[SPECIAL_SEPARATOR_TOKEN]'),
        batch_size: vh.mkoption(int, default=100),
        max_input_len: vh.mkoption(int, nargs='?'),
        max_output_len: vh.mkoption(int, nargs='?'),
        use_mapper: vh.mkoption(bool, default=False),
        mapper_jobs: vh.mkoption(int, default=-1),
        yt_pool: vh.mkoption(str, default='nirvana-dialogs')
):
    model_folder = 'model_folder'
    hp = unpack_model_with_hp(model_archive, model_folder)

    generate_bucket(
        input_table=str(contexts_table), output_table=str(output_table),
        yt_proxy='hahn',
        model_class='tfnn.task.seq2seq.models.transformer.Model',
        model_path=os.path.join(model_folder, 'model.npz'),
        token_to_id_voc_path=os.path.join(model_folder, 'token_to_id.voc'),
        bpe_voc_path=os.path.join(model_folder, 'bpe.voc'),
        hp=hp,
        separator_token=separator_token,
        batch_size=batch_size,
        sampling_temperature=sampling_temperature,
        sampling_hypothesis_per_input=num_hypos,
        sampling_topk=sampling_topk,
        unique_hypotheses_per_input=unique_hypotheses_per_input,
        process_n_first_rows=None if process_n_first_rows == -1 else process_n_first_rows,
        contexts_columns=contexts_columns,
        max_input_len=max_input_len[0] if len(max_input_len) > 0 else None,
        max_output_len=max_output_len[0] if len(max_output_len) > 0 else None,
        use_mapper=use_mapper,
        mapper_jobs=mapper_jobs,
        yt_pool=yt_pool
    )
    return output_table


@vh.lazy.arcadia_executable_path(VH_DUMMY_EXECUTABLE_PATH, build_flags=TENSORFLOW_GPU_FLAGS)
@vh.lazy.hardware_params(DEFAULT_GPU_HARDWARE)
@vh.lazy.from_annotations
def score_bucket_op(
        contexts_table: vh.YTTable,
        model_archive: vh.File,
        output_table: vh.mkoutput(vh.YTTable),
        sampling_temperature: vh.mkoption(float, default=0.6),
        reply_column: vh.mkoption(str),
        tokenize_reply_column: vh.mkoption(bool),
        context_columns_prefix: vh.mkoption(str, default='context'),
        process_n_first_rows: vh.mkoption(int, default=-1)
):
    model_folder = 'model_folder'
    hp = unpack_model_with_hp(model_archive, model_folder)

    score_bucket(
        contexts_table, output_table, yt_proxy='hahn',
        model_class='tfnn.task.seq2seq.models.transformer.Model',
        model_path=os.path.join(model_folder, 'model.npz'),
        token_to_id_voc_path=os.path.join(model_folder, 'token_to_id.voc'),
        bpe_voc_path=os.path.join(model_folder, 'bpe.voc'),
        hp=hp,
        separator_token='[SPECIAL_SEPARATOR_TOKEN]',
        sampling_temperature=sampling_temperature,
        reply_column=reply_column,
        tokenize_reply_column=tokenize_reply_column,
        context_columns_prefix=context_columns_prefix,
        process_n_first_rows=None if process_n_first_rows == -1 else process_n_first_rows
    )
