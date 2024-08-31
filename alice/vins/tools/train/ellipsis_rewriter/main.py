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
            archive.add('model.py')
            archive.add('train.py')
            archive.add('train_config.json')

        embeddings = sky_get_op(rbtorrent='sbr:1018047534', max_disk=3000)
        train_data = sky_get_op(rbtorrent='sbr:1798944666')
        data = concat_archives_op(input_1=embeddings, input_2=train_data, max_disk=3000)

        porto_layer = sky_get_op(rbtorrent='sbr:1798050585', max_disk=2000)

        dl_op(
            _inputs={
                'script': trainer_script_path,
                'data': data,
                'volume': porto_layer
            },
            _options={
                'run_command': 'mv $SOURCE_CODE_PATH/train_config.json $INPUT_PATH\n\n'
                               '/opt/conda/envs/tf/bin/python $SOURCE_CODE_PATH/train.py --data-dir $INPUT_PATH '
                               '--embeddings-dir $INPUT_PATH --output-path $SNAPSHOT_PATH',
                'gpu-type': 'CUDA_7_0',
                'gpu-count': 1,
                'gpu-max-ram': 30000,
                'max-ram': 32000,
            }
        )['state']

        vh.run(api_retries=100)
    finally:
        if tmpdir is not None:
            shutil.rmtree(tmpdir)


if __name__ == '__main__':
    main()
