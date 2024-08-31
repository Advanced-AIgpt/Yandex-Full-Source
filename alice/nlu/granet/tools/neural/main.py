# coding: utf-8

import argparse
import os
import logging
import tarfile
import tempfile
import shutil
import vh

logger = logging.getLogger(__name__)

_TRAIN_CMD = (
    'python $SOURCE_CODE_PATH/train.py '
    '--positives-path $SOURCE_CODE_PATH/positives_train.tsv '
    '--negatives-path $SOURCE_CODE_PATH/negatives_train.tsv '
    '--embeddings-dir $INPUT_PATH '
    '--output-path $SNAPSHOT_PATH/model'
)

_APPLY_CMD = (
    'python $SOURCE_CODE_PATH/apply_model.py '
    '--positives-path $SOURCE_CODE_PATH/positives_test.tsv '
    '--negatives-path $SOURCE_CODE_PATH/negatives_test.tsv '
    '--tagger-path $SNAPSHOT_PATH/model '
    '--embeddings-dir $INPUT_PATH '
    '--num-procs 16 '
    '--output-path $SNAPSHOT_PATH/prediction'
)


def main():
    logging.basicConfig(level=logging.INFO, format='[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s')

    parser = argparse.ArgumentParser()
    parser.add_argument('--train-positives-path', required=True)
    parser.add_argument('--train-negatives-path', required=True)
    parser.add_argument('--test-positives-path', required=True)
    parser.add_argument('--test-negatives-path', required=True)

    args = parser.parse_args()

    sky_get_op = vh.op(name='sky get', owner='timofeich')
    dl_op = vh.op(name='Python Deep Learning', owner='dudevil')

    tmpdir = None
    try:
        tmpdir = tempfile.mkdtemp()
        trainer_script_path = os.path.join(tmpdir, 'trainer.tar')
        with tarfile.open(trainer_script_path, 'w') as archive:
            archive.add('apply_model.py')
            archive.add('model.py')
            archive.add('train.py')
            archive.add('utils.py')
            archive.add(args.train_positives_path, 'positives_train.tsv')
            archive.add(args.train_negatives_path, 'negatives_train.tsv')
            archive.add(args.test_positives_path, 'positives_test.tsv')
            archive.add(args.test_negatives_path, 'negatives_test.tsv')

        embeddings = sky_get_op(rbtorrent='sbr:1018047534', max_disk=3000)

        dl_op(
            _inputs={
                'script': trainer_script_path,
                'data': embeddings
            },
            _options={
                'run_command': '{} && {}'.format(_TRAIN_CMD, _APPLY_CMD),
                'gpu-type': 'CUDA_6_1',
                'gpu-count': 1,
                'gpu-max-ram': 30000,
                'max-ram': 32000,
                'pip': [
                    'attrs==19.1.0',
                    '-i https://pypi.yandex-team.ru/simple vins-models-tf==0.5.1'
                ]
            }
        )['state']

        vh.run(
            quota='voice-core'
        )
    finally:
        if tmpdir is not None:
            shutil.rmtree(tmpdir)


if __name__ == '__main__':
    main()
