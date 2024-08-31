import binascii
import json
import os
from pathlib import Path

from library.python.vault_client.instances import Production

from library.python.svn_version import commit_id, svn_branch


WORKDIR = Path.cwd() / 'workdir'


def workdir():
    return WORKDIR


def arcadia_path(path):
    cwd = Path.cwd()

    while cwd.name:
        if (cwd / '.arcadia.root').exists():
            return cwd / path
        cwd = cwd.parent

    raise ValueError(f'failed to find Arcadia root from cwd {Path.cwd()}')


def bin_path(name):
    return arcadia_path(f'jupytercloud/backend/bin/{name}/{name}')


def tvm_local_token():
    path = WORKDIR / '.tvm_local_token'

    if not path.exists():
        token = binascii.b2a_hex(os.urandom(16)).decode('utf-8')
        path.write_text(token)

    token = path.read_text()
    return token.strip()


def tvm_config(secret_id, self_tvm_id):
    path = WORKDIR / '.tvm.json'

    if not path.exists():
        secret = get_secrets(secret_id)['client_secret'].strip()

        config = {
            'BbEnvType': 2,
            'clients': {
                'jupytercloud_test': {
                    'secret': secret,
                    'self_tvm_id': self_tvm_id,
                    'dsts': {
                        'blackbox': {
                            'dst_id': 223,
                        },
                    },
                },
            },
            'port': 18080,
        }

        with path.open('w') as f_:
            json.dump(config, f_)

    return str(path)


def get_secrets(secret_id):
    vault_client = Production(decode_files=True)
    return vault_client.get_version(secret_id)['value']


def get_version():
    return f'{commit_id()}.{svn_branch()}'


def id_rsa(secret_id):
    path = WORKDIR / 'id_rsa'

    if not path.exists():
        secrets = get_secrets(secret_id)
        value = secrets['id_rsa']
        path.write_bytes(value)

    os.chmod(str(path), 0o0600)

    return str(path)
