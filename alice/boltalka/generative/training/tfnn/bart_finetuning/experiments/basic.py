import os

import vh

from alice.boltalka.generative.training.data.nn.util.ops import mr_copy, mr_shuffle, merge_tables
from dict.mt.make.libs.common import VhRunner
from ..util import get_training_options, get_training_block, get_dev_corpus, get_bart_vocs, \
    get_bart_pretrain


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
        for use_state in [False, True]:
            yield train_table_fn, experiment_label + ('_nostate' if not use_state else ''), use_state, val_tables


def run():
    for train_table_fn, experiment_label, use_state, val_tables in get_configs():
        with VhRunner(backend='nirvana') as runner, vh.HardwareParams(
                ttl=360000,
                max_ram=2000
        ):
            train_block = get_training_block()
            token_to_id_voc, bpe_voc = get_bart_vocs()
            bart_model, bart_state = get_bart_pretrain()

            train_options = get_training_options(
                bart_model, bart_state,
                steps_offset=980000 if use_state else 0,
                checkpoint_every_steps=500,
                keep_checkpoints_max=2,
                save_best_dev_checkpoint_config={
                    'max_times_unchanged_dev_value': 3,
                    'dev_name': 'val_twitter_respect'
                }
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
            )

            runner.run(
                label=experiment_label,
                arcadia_revision=7611431,
            )
