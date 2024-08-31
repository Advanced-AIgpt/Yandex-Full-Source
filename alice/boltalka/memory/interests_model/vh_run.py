import sys
sys.path.append('../../rl/valhalla/')
from vh_dl import *

if __name__ == "__main__":
    arcadia_path = '/mnt/storage/alzaharov/arcadia'
    data_path = '/mnt/storage/nzinov/rl'

    scripts = [
        'alice/boltalka/memory/interests_model/train.py',
        'alice/boltalka/memory/interests_model/model.py',
        'alice/boltalka/memory/interests_model/dataset.py',
        'alice/boltalka/rl/experiment.py',
        'alice/boltalka/rl/util.py'
    ]
    name = "il1024_softmax_lr1e-4_bc32"

    data = []

    commands = (
        'KMP_DUPLICATE_LIB_OK=TRUE YT_PROXY=hahn python3.6 $SOURCE_CODE_PATH/train.py --name ' + name,
    )

    porto_options = {
        'sandbox_oauth_token': 'alzaharov_sandbox_token'
    }

    pdl_options = {
        'master-gpu-count': 1,
        'yt-token': 'alzaharov_yt_token',
    }
    label = 'dssm-lstm {}'.format(name)
    vh_train(arcadia_path, data_path, scripts, data,
             commands, porto_options, pdl_options, label)
