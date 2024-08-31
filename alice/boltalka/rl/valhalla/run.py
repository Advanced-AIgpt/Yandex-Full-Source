import tarfile
import vh
import os
import requests

ARCADIA_PATH = '/mnt/storage/nzinov/arc/arcadia'
DATA_PATH = '/mnt/storage/nzinov/rl'

SCRIPTS = [
    'alice/boltalka/rl/q_simulator.py',
    'alice/boltalka/rl/experiment.py',
    'alice/boltalka/rl/off_policy_eval.py',
    'alice/boltalka/rl/data_loader.py',
    'alice/boltalka/rl/gym.py',
    'alice/boltalka/rl/util.py',
    'alice/boltalka/py_libs/nlgsearch_simple/nlgsearch.so'
]

DATA = [
    'starters.txt'
]

if True:
    with tarfile.open('script.tar', 'w', dereference=True) as tar:
        for f in SCRIPTS:
            tar.add(os.path.join(ARCADIA_PATH, f), os.path.basename(f))
    with tarfile.open('data.tar', 'w', dereference=True) as tar:
        for f in DATA:
            tar.add(os.path.join(DATA_PATH, f), os.path.basename(f))
script = vh.File('script.tar')
data = vh.File('data.tar')

PDL = vh.op(id="190dedba-a00d-4073-91b3-e80ae2503455")

commands = (
    'cp $INPUT_PATH/* $TMPFS_DIR/',
    'sky get -d $TMPFS_DIR -w rbtorrent:62e36cef51d2bf8a3651783bcf95b5fb0507844a',
    'INPUT_PATH=$TMPFS_DIR KMP_DUPLICATE_LIB_OK=TRUE YT_PROXY=hahn python3.6 $SOURCE_CODE_PATH/q_simulator.py --name valhalla',
)

build_porto_layer = vh.op(id="0164ff39-6fd8-483b-a631-ef20a936e0a2")

layer = build_porto_layer(
    _name="""Build Porto Layer""",
    _options={
        'script': 'sudo apt-get update\nsudo apt-get install -y software-properties-common\nsudo add-apt-repository ppa:deadsnakes/ppa\nsudo apt-get update\nsudo apt-get install -y python3.6 python3.6-dev\n\nwget -q -O - https://bootstrap.pypa.io/get-pip.py | sudo python3.6\n\npython3.6 -m pip install -U numpy torch torchvision tensorflow-gpu tqdm pandas scipy scikit-learn matplotlib tensorboardX\n\npython3.6 -m pip install -U --index-url https://pypi.yandex-team.ru/simple yandex-global-state-controller yandex-yt-yson-bindings\n\nsudo rm /usr/local/bin/pip',
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

pdl_cube = PDL(
    _name="""train""",
    _inputs={
        'data': data,
        'script': script,
        'volume': layer
    },
    _options={
        'run_command': ' && '.join(commands),
        'master-cpu-cores': 54,
        'master-gpu-count': 0,
        'openmpi_runner': False,
        'ttl': 14400,
        'master-max-ram': 50000,
        'master-max-tmpfs-disk': 50000,
        'master-max-disk': 100000,
        'master-gpu-type': 'CUDA_ANY',
        'master-gpu-max-ram': 0,
        'yt-token': 'nzinov_yt_token',
        'mr-account': 'voice',
        'pip': ['--no-deps -U --extra-index-url https://pypi.yandex-team.ru/simple yandex-global-state-controller yandex-yt-yson-bindings'],
        'auto_snapshot': 10,
        'slaves': 0,
        'job-is-vanilla': False,
        'retries-on-job-failure': 0,
        'timestamp': '2019-03-11T19:58:35+0300',
        'debug-timeout': 0,
    },
)

vh.run(quota='voice-core')
