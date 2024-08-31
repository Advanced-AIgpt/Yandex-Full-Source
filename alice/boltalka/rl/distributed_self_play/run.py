import tarfile
import vh
import os
import sys
import time
from alice.boltalka.rl.distributed_self_play.lib import run_training, run_generation

ARCADIA_PATH = '/mnt/storage/nzinov/arc/arcadia'
DATA_PATH = '/mnt/storage/nzinov/rl'
NAME = 'distributed-self-play'

SCRIPTS = [
    'alice/boltalka/rl/experiment.py',
    'alice/boltalka/rl/distributed_self_play/util.py',
    'alice/boltalka/rl/distributed_self_play/models.py',
    'alice/boltalka/rl/distributed_self_play/train.py',
    'alice/boltalka/rl/distributed_self_play/session_sampler.py',
    'alice/boltalka/rl/distributed_self_play/mlockall.py',
    'alice/boltalka/py_libs/nlgsearch_simple/nlgsearch.so'
]

DATA = [
    'starters.txt'
]


build_porto_layer = vh.op(id="0164ff39-6fd8-483b-a631-ef20a936e0a2")


def main():
    comment = sys.argv[1] if len(sys.argv) > 1 else ''

    layer = build_porto_layer(
        _name="""Build Porto Layer""",
        _options={
            'script': '''sudo apt-get update
sudo apt-get install -y software-properties-common
sudo add-apt-repository ppa:deadsnakes/ppa
sudo apt-get update
sudo apt-get install -y python3.6 python3.6-dev

wget -q -O - https://bootstrap.pypa.io/get-pip.py | sudo python3.6

python3.6 -m pip install -U pyzmq numpy torch torchvision tensorflow-gpu tqdm pandas scipy scikit-learn matplotlib tensorboardX asyncpool

python3.6 -m pip install -U --index-url https://pypi.yandex-team.ru/simple yandex-global-state-controller yandex-yt-yson-bindings

sudo rm /usr/local/bin/pip''',
            'parent_layer': '489229145',
            'layer_name': 'pdl_py3.6',
            'sandbox_requirements_ram': 16000,
            'sandbox_requirements_disk': 16000,
            'script_sandbox_owner': 'VINS',
            'compress': 'tar.gz',
            'space_limit': 16000,
            'memory_limit': 16000,
            'merge_layers': False,
            'sandbox_oauth_token': 'nzinov_sandbox_token'
        },
    )['layer']

    if True:
        with tarfile.open('script.tar', 'w', dereference=True) as tar:
            for f in SCRIPTS:
                tar.add(os.path.join(ARCADIA_PATH, f), os.path.basename(f))
        with tarfile.open('data.tar', 'w', dereference=True) as tar:
            for f in DATA:
                tar.add(os.path.join(DATA_PATH, f), os.path.basename(f))

    script = vh.File('script.tar')
    data = vh.File('data.tar')
    '''
    script = vh.data(id='0e6fb627-01d8-43db-8923-d0621a3f2a1a')
    data = vh.data(id='7406dcc8-7f7c-4d5d-8d8e-93c82860d513')
    '''

    blocks = []
    if False:
        with vh._uop_options({
                'slave-job-variables': ['Y_PYTHON_ENTRY_POINT=:main', 'TMPFS_PATH=${tmpfs_path}'],
                'slave-max-tmpfs-disk': 70000,
                'job-is-vanilla': False,
                }):
            training = run_training(comment, data, script)
            training.set_container(vh.Porto(layer))
            blocks.append(training)

    else:
        timestamp = int(time.time())
        for i in range(10):
            with vh._uop_options({
                    'job-variables': ['Y_PYTHON_ENTRY_POINT=:main', 'TMPFS_PATH=${tmpfs_path}'],
                    'max-tmpfs-disk': 70000,
                    'job-is-vanilla': False,
                    }):
                training = run_generation(timestamp + i, data, script)
                training.set_container(vh.Porto(layer))
                blocks.append(training)

    vh.run(quota='dialogs', label='[distRL] ' + comment)
