import os

import vh
from alice.boltalka.generative.pipelines.analysis.analyze_buckets import analyze_buckets
from alice.boltalka.generative.pipelines.calc_informativeness import calc_informativeness
from alice.boltalka.generative.pipelines.factor_dssm.calc_factor_dssm import calc_factor_dssm
from alice.boltalka.generative.pipelines.measure_on_toloka import measure_interestingness, measure_ds_score
from alice.boltalka.generative.tfnn.bucket_maker.vh import generate_bucket_op
from alice.boltalka.generative.training.data.nn.util.ops import mr_upload, mr_copy
from dict.mt.make.libs.override import override
from dict.mt.make.libs.types.corpus import DevCorpus
from dict.mt.make.libs.types.tfnn import (
    ModelDescriptor,
    OptimizerDescriptor,
    DataReaderDescriptor,
    SaveLoadOptions,
    DevOptions,
    SummaryOptions,
    TrainOptions,
)
from dict.mt.make.modules.corpus import read_yt_column
from dict.mt.make.modules.tfnn import TrainBlock


def get_training_block(n_gpus=8):
    train_block = TrainBlock(gpu_count=n_gpus, stream_train_from_yt=True)

    train_block = override(train_block, {
        'run_command_with_deps.nirvana_op_wrapper.ssh_key_secret_name': None,
    })
    return train_block


def get_training_options(
        bart_model,
        bart_state=None,
        checkpoint_every_steps=500,
        score_dev_every=500,
        keep_checkpoints_max=2,
        save_best_dev_checkpoint_config={},
        steps_offset=980 * 1000,
        steps_after_offset=20000,
        sync_every_steps=1
):
    model_descriptor = ModelDescriptor(hp={
        'ff_size': 5120,
        'rescale_emb': True,
        'num_layers': 24,
        'res_steps': 'nlda',
        'vtype': 'float16',
        'hid_size': 1280,
        'inp_emb_bias': False,
        'normalize_out': True,
        'num_heads': 20,
        'emb_size': 1280,
        'share_emb': True,
        'dwwt': True,

        'emb_in_device': '',
        'emb_out_device': '',
        'relu_dropout': 0.1,
        'attn_dropout': 0.1,
        'label_smoothing': 0.1,
    })
    optimizer_descriptor = OptimizerDescriptor(
        klass='adam',
        opts={
            'adaptive_loss_multiplier_steps': 2000,
            'clip_norm': 0,
            'beta2': 0.998,
            'beta1': 0.9,
            'epsilon': 1e-8,
            'loss_multiplier': 1e4,
            'average_grads': True,
            'aggregate_dtype': 'float16',
            'nullify_nan_gradients': True
        },
        learning_rate='scale * linear_warmup(warmup_steps) * rsqrt_decay(since_step=warmup_steps)',
        learning_rate_opts={
            'warmup_steps': 10000,
            'scale': 0.00014,
        },
    )
    data_reader_descriptor = DataReaderDescriptor(
        opts={
            'max_srclen': 512,
            'maxlen_min': 8,
            'batch_maker': 'adaptive_windowed',
            'batch_len': 512,
            'split_len': 10000,
            'maxlen_quant': 1,
            'max_dstlen': 512,
            'batch_size_max': 0,
            'shuffle_len': 10000,
            'split_alterate': False,
            'loaders': ('str', 'str'),
        },
    )
    train_options = TrainOptions(
        seed=42,
        num_batches=steps_offset + steps_after_offset,
        sync_every_steps=sync_every_steps,
        model_descriptor=model_descriptor,
        optimizer_descriptor=optimizer_descriptor,
        data_reader_descriptor=data_reader_descriptor,
        save_load_options=SaveLoadOptions(
            pre_init_model_checkpoint=bart_model,
            pre_init_state_checkpoint=bart_state,
            adapt_mismatched_shapes_at_load=True,
            checkpoint_every_steps=checkpoint_every_steps,
            keep_checkpoints_max=keep_checkpoints_max,
            save_best_dev_checkpoint_config=save_best_dev_checkpoint_config
        ),
        dev_options=DevOptions(
            score_dev_every=score_dev_every,
            translate_dev=False,
            convergence_tracker_opts=None
        ),
        summary_options=SummaryOptions(
            params_summary_every_steps=1000000000,
            summary_every_steps=50,
        ),
        aux_params={
            '--session-opts': {
                'gpu_memory_fraction': 0.98,
            },
        },
        gpu_type='CUDA 7.0 GPU',
        gpu_max_ram=30000,
    )

    return train_options


def get_dev_corpus(table, columns):
    return DevCorpus(name='val', aux_columns=columns), tuple(read_yt_column(c, vh.YTTable(table)) for c in columns)


def get_bart_vocs():
    voc_file = vh.get_yt_file('//home/voice/artemkorenev/boltalka/bart_lm/token_to_id_with_separator.voc')
    bpe_voc = vh.get_yt_file('//home/voice/artemkorenev/boltalka/bart_lm/bpe.voc')
    return voc_file, bpe_voc


def get_bart_pretrain():
    bart_model = vh.get_yt_file('//home/voice/artemkorenev/bart_980k/model.npz')
    bart_state = vh.get_yt_file('//home/voice/artemkorenev/bart_980k/state.npz')
    return bart_model, bart_state


@vh.cwd
def measure_bundle_model(
        model_bundle,
        out_folder_path,
        contexts_table='//home/voice/boltalka/buckets/val_bucket_20200604',
        contexts=['context_{}'.format(i) for i in reversed(list(range(9)))],
        do_measure_ds_score=True,
        do_measure_interestingness=True,
        do_measure_informativeness=True,
        do_measure_factor_dssm=True,
):
    contexts_table = vh.YTTable(contexts_table)

    with vh.HardwareParams(max_disk=60000, max_ram=50000):
        bucket = vh.YTTable(os.path.join(out_folder_path, 'bucket'))
        bucket = generate_bucket_op(
            contexts_table, model_bundle, bucket,
            contexts_columns=contexts, batch_size=1,
            max_input_len=128, max_output_len=128, process_n_first_rows=1000
        )

    if do_measure_ds_score:
        report, table_ds = measure_ds_score(bucket, yt_token=vh.get_yt_token_secret())
        mr_copy(table_ds, os.path.join(out_folder_path, 'bucket_relevance'))
        mr_upload(report, os.path.join(out_folder_path, 'report_relevance'))

    if not vh.has_global_option('yql-token-secret'):
        vh.add_global_option('yql-token-secret', type=vh.Secret)
    if do_measure_interestingness:
        table_int, report = measure_interestingness(
            bucket, yt_token=vh.get_yt_token_secret(), yql_token=vh.get_global_option('yql-token-secret')
        )
        mr_copy(table_int, os.path.join(out_folder_path, 'bucket_interestingness'))
        mr_upload(report, os.path.join(out_folder_path, 'report_interestingness'))

    inf_table = None
    if do_measure_informativeness:
        inf_table = calc_informativeness(bucket, yt_token=vh.get_yt_token_secret())
        mr_copy(inf_table, os.path.join(out_folder_path, 'bucket_informativeness'))

    factor_table = None
    if do_measure_factor_dssm:
        factor_table = calc_factor_dssm(bucket)
        mr_copy(factor_table, os.path.join(out_folder_path, 'bucket_factor'))

    report = analyze_buckets(bucket, informativeness_bucket=inf_table, dssm_factor_bucket=factor_table)
    mr_upload(report, os.path.join(out_folder_path, 'report_analysis'))
