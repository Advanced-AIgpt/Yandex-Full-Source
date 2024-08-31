# coding: utf-8

import os
import logging
import tarfile
import tempfile
import shutil
import vh

logger = logging.getLogger(__name__)


def main():
    logging.basicConfig(level=logging.INFO, format='[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s')

    sky_get_op = vh.op(name='sky get', owner='timofeich')
    concat_archives_op = vh.op(name='Concatenate tar archives', owner='dan-anastasev')
    dl_op = vh.op(name='Python Deep Learning', owner='splinter')

    tmpdir = None
    try:
        tmpdir = tempfile.mkdtemp()
        trainer_script_path = os.path.join(tmpdir, 'trainer.tar')
        with tarfile.open(trainer_script_path, 'w') as archive:
            archive.add('evaluate_predictions.py')
            archive.add('model.py')
            archive.add('train.py')
            archive.add('train_config.json')
            archive.add('utils.py')

        embeddings = sky_get_op(rbtorrent='sbr:1018047534', max_disk=3000)
        train_data = sky_get_op(rbtorrent='sbr:1157060406')
        data = concat_archives_op(input_1=embeddings, input_2=train_data)

        dl_op(
            _inputs={
                'script': trainer_script_path,
                'data': data
            },
            _options={
                'run_command': 'python $SOURCE_CODE_PATH/train.py --data-path $INPUT_PATH --output-path $SNAPSHOT_PATH',
                'gpu-type': 'CUDA_6_1',
                'gpu-count': 1,
                'gpu-max-ram': 30000,
                'max-ram': 32000,
                'pip': ['attrs==19.1.0', 'scikit-learn==0.20.3', 'typing==3.6.6']
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
