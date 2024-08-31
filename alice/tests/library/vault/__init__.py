import os
import logging

from library.python.vault_client.instances import Production as VaultClient


SECRET_UUID = 'sec-01e3pkyeyhretd6d1gqjjq9625'

CREDENTIALS_PREFIXES = {
    'oauth_token': '',
    'uid': 'uid_',
}


def _read_from_file(filename):
    with open(filename) as stream:
        return stream.read().strip()


def _get_yav_token(yav_token_key='YAV_TOKEN'):
    if yav_token_key not in os.environ:
        return None

    logging.info(f'{yav_token_key} found in environ')
    token = os.environ[yav_token_key]
    if os.path.exists(token):
        token = _read_from_file(token)

    return f'OAuth {token}'


def get_oauth_token(username, secret_uuid=None):
    secret_uuid = secret_uuid or SECRET_UUID
    client = VaultClient(decode_files=True, authorization=_get_yav_token())
    return client.get_version(secret_uuid)['value'][username]


def get_credentials(username, secret_uuid=None):
    secret_uuid = secret_uuid or SECRET_UUID
    client = VaultClient(decode_files=True, authorization=_get_yav_token())
    secret_value = client.get_version(secret_uuid)['value']
    credentials = {}
    for cred, prefix in CREDENTIALS_PREFIXES.items():
        credentials[cred] = secret_value[prefix + username]
    return credentials
