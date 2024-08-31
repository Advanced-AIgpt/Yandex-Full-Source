import json
import os

import vh
from alice.boltalka.generative.pipelines.staging.converting.convert import training_checkpoint_to_bundle
from alice.boltalka.generative.training.data.nn.util.ops import mr_copy, mr_shuffle, merge_tables, mr_mkdir, mr_upload
from dict.mt.make.libs.common import VhRunner
from ..util import get_training_options, get_training_block, get_dev_corpus, get_bart_vocs, \
    get_bart_pretrain, measure_bundle_model


def get_configs():
    data_folder = '//home/voice/artemkorenev/boltalka'
    twitter_folder = os.path.join(data_folder, 'twitter_filtered_whitelist/bert_and_respect')
    assessors_folder = os.path.join(data_folder, 'assessors_new')

    val_tables = {
        'val_twitter_respect': os.path.join(twitter_folder, 'val'),
        'val_assessors': os.path.join(assessors_folder, 'val'),
    }

    for train_table_fn, experiment_label in [
        (
                lambda: vh.YTTable(os.path.join(twitter_folder, 'train')),
                'twitter_respect_and_bert',
        ),
        (
                lambda: vh.YTTable(os.path.join(assessors_folder, 'train')),
                'assessors'
        ),
        (
                lambda: mr_copy(
                    mr_shuffle(merge_tables([
                        vh.YTTable(os.path.join(twitter_folder, 'train')),
                        vh.YTTable(os.path.join(assessors_folder, 'train'))
                    ])),
                    os.path.join(twitter_folder, '_tmp_twitter_and_assessors_merged')
                ),
                'twitter_and_assessors_merged'
        )
    ]:
        yield train_table_fn, experiment_label + '_multi', val_tables


def run():
    for train_table_fn, experiment_label, val_tables in get_configs():
        with VhRunner(backend='nirvana') as runner, vh.HardwareParams(
                ttl=360000,
                max_ram=2000
        ):
            train_block = get_training_block()
            token_to_id_voc, bpe_voc = get_bart_vocs()
            bart_model, bart_state = get_bart_pretrain()

            start_step = 980000
            measure_metrics_every = 5000
            n_metrics_measurements = 4
            train_options = get_training_options(
                bart_model, bart_state,
                checkpoint_every_steps=measure_metrics_every,
                score_dev_every=measure_metrics_every,
                save_best_dev_checkpoint_config=None,
                keep_checkpoints_max=n_metrics_measurements + 1,
                steps_offset=start_step,
                steps_after_offset=measure_metrics_every * n_metrics_measurements
            )

            COLUMNS = ['context_0', 'reply']
            val_corpora = dict()
            for corpora_name, table_path in val_tables.items():
                val_corpora[corpora_name] = get_dev_corpus(table_path, COLUMNS)

            train_table = train_table_fn()
            trained_state = train_block(
                token_to_id_voc,  # src_voc: vh.File
                token_to_id_voc,  # dst_voc: vh.File
                train_table,  # train_corpus: vh.YTTable
                COLUMNS,  # train_columns: Sequence[str]
                val_corpora,  # dev_corpora: Dict[str, Sequence[vh.File]]
                train_options,  # train_options: TrainOptions
            ).state

            folder = '//home/voice/artemkorenev/boltalka/pipelines/_experiments/bart/periodic_measuremnts/{}'.format(
                experiment_label
            )
            for i in range(n_metrics_measurements):
                offset = start_step + (i + 1) * measure_metrics_every
                with vh.HardwareParams(max_disk=150000):
                    bundle = training_checkpoint_to_bundle(
                        trained_state,
                        bpe_file=bpe_voc, token_to_id_file=token_to_id_voc,
                        hp_file=vh.data_from_str(json.dumps(train_options.model_descriptor.hp)),
                        model_filename='model-{}.npz'.format(int(offset))
                    )

                cwd = 'steps_{}'.format(int(offset))
                target_folder = os.path.join(folder, cwd)
                mr_mkdir(target_folder)
                mr_upload(bundle, os.path.join(target_folder, 'bundle'))

                measure_bundle_model(bundle, out_folder_path=target_folder)

            runner.run(
                label=experiment_label,
                arcadia_revision=7611431,
            )
