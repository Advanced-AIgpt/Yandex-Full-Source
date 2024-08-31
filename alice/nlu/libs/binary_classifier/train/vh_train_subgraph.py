# coding: utf-8

import json
import vh

from global_options import nirvana_global_options


_TRAIN_COMMAND = (
    'python $SOURCE_CODE_PATH/train.py --config-path $SOURCE_CODE_PATH/config.json '
    '--output-dir $SNAPSHOT_PATH/model '
    '&& mv $SNAPSHOT_PATH/model/model.pb $SNAPSHOT_PATH/ '
    '&& mv $SNAPSHOT_PATH/model/model_description.json $SNAPSHOT_PATH/ '
    '&& rm -r $SNAPSHOT_PATH/model'
)


def _build_train_script_loading_block(config_path):
    svn_checkout_op = vh.op(name='SVN: Checkout (Deterministic)', owner='finiriarh')
    echo_op = vh.op(name='Echo to TSV, JSON, TEXT', owner='mstebelev')
    add_to_archive_op = vh.op(name='Add file to tar.gz', owner='paulkovalenko')

    trainer_script = svn_checkout_op(
        _options={'arcadia_path': 'arcadia/alice/nlu/libs/binary_classifier/train', 'revision': 7447738}
    )
    with open(config_path) as f:
        config = json.load(f)
    config = echo_op(input=json.dumps(config, indent=2))

    trainer_script = add_to_archive_op(
        _inputs={'archive': trainer_script, 'file': config},
        _options={'path': 'config.json'}
    )

    return trainer_script


def _build_py_dl_block(config_path, data_table):
    dl_op = vh.op(name='Python Deep Learning', owner='splinter')
    table_to_json_op = vh.op(name='MR Table to json', owner='dan-anastasev')

    trainer_script = _build_train_script_loading_block(config_path)

    params = table_to_json_op(data_table)

    state = dl_op(
        _inputs={
            'script': trainer_script,
            'params': params,
        },
        _options={
            'run_command': _TRAIN_COMMAND,
            'gpu-type': 'CUDA_7_0',
            'gpu-count': 1,
            'gpu-max-ram': 30000,
            'max-ram': 32000,
            'yt-token': nirvana_global_options.yt_token,
            'pip': [
                'attrs==19.1.0',
                'click==7.0',
                'pydl-tensorflow',
                'tqdm==4.48.0',
                'yandex-yt-yson-bindings==0.3.31.post0',
                'yandex-yt==0.9.23',
            ]
        }
    )['state']

    return state


def _build_upload_to_sandbox_block(state):
    upload_op = vh.op(name='ya upload to sandbox', owner='sameg')
    upload_op(
        _inputs={
            'data': state
        },
        _options={
            'type': 'OTHER_RESOURCE',
            'user': nirvana_global_options.sandbox_owner,
            'token': nirvana_global_options.sandbox_token,
            'do_not_remove': True
        }
    )


def build_train_subgraph(config_path, data_table):
    state = _build_py_dl_block(config_path, data_table)
    _build_upload_to_sandbox_block(state)
