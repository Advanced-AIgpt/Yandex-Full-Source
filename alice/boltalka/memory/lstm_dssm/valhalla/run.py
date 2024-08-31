import tarfile
import vh
import os
import requests

ARCADIA_PATH = '/mnt/storage/alzaharov/arcadia'
DATA_PATH = '/mnt/storage/nzinov/rl'

SCRIPTS = [
    'alice/boltalka/memory/lstm_dssm/train.py',
    'alice/boltalka/memory/lstm_dssm/lstm_dssm_model.py',
    'alice/boltalka/rl/experiment.py',
    'alice/boltalka/rl/util.py'
]

DATA = []

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
name = "l256_fc300_2_cos_lr1e-5_bc32_stupid_embeds_other_reply"
commands = (
    'KMP_DUPLICATE_LIB_OK=TRUE YT_PROXY=hahn python3.6 $SOURCE_CODE_PATH/train.py --name ' + name,
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
        'sandbox_oauth_token': 'alzaharov_sandbox_token'
    },
)['layer']

pdl_cube = PDL(
    _name="""train {}""".format(name),
    _inputs={
        'data': data,
        'script': script,
        'volume': layer
    },
    _options={
        'run_command': ' && '.join(commands),
        'master-cpu-cores': 54,
        'master-gpu-count': 1,
        'openmpi_runner': False,
        'ttl': 14400,
        'master-max-ram': 50000,
        'master-max-tmpfs-disk': 50000,
        'master-max-disk': 100000,
        'master-gpu-type': 'CUDA_ANY',
        'master-gpu-max-ram': 0, p
        'yt-token': 'alzaharov_yt_token',
        'mr-account': 'voice',
        'pip': ['--no-deps -U --extra-index-url https://pypi.yandex-team.ru/simple yandex-global-state-controller yandex-yt-yson-bindings'],
        'auto_snapshot': 10,
        'slaves': 0,
        'job-is-vanilla': False,
        'retries-on-job-failure': 0,
        'timestamp': '2019-03-11T19:58:35+0300',
        'debug-timeout': 1000,
    },
)

vh.run(quota='voice-core', label='dssm-lstm {}'.format(name))
