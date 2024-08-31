# -*- coding: utf-8 -*-

from __future__ import absolute_import, print_function, division, unicode_literals

import os
import logging
import warnings

from paramiko.rsakey import RSAKey
from paramiko.ssh_exception import SSHException, PasswordRequiredException

from library.python.vault_client.auth import BaseRSAAuth
from library.python.vault_client.instances import Production

from .compat import Path, cached_property
from .operation import is_nirvana

__all__ = [
    'get_secret',
    'get_secret_key'
]

logger = logging.getLogger(__name__)


class HomeSSHRSAAuth(BaseRSAAuth):
    ssh_dir = '~/.ssh'

    def __call__(self):
        keys = []

        ssh_dir = Path(self.ssh_dir).expanduser()
        if not ssh_dir.exists():
            warnings.warn(
                "trying to find ssh RSA keys in directory {}, but it does not exists"
                .format(ssh_dir),
                RuntimeWarning
            )
            return keys

        ssh_key_candidates = ssh_dir.glob('*')

        for key_path in ssh_key_candidates:
            try:
                # python2 paramiko do not eat Path object
                key = RSAKey.from_private_key_file(str(key_path))
                keys.append(key)
            except PasswordRequiredException:
                logger.debug('key file %s requires password, skip it', key_path)
            # paramiko doesn't normally process some files and falls with IndexError
            except (SSHException, IndexError):
                logger.debug('ssh key file candidate %s is not a valid RSA file, skip it', key_path)

        if not keys:
            warnings.warn(
                "couldn't find any suitable RSA key in directory {}".format(ssh_dir),
                RuntimeWarning
            )

        return keys


class JupyterYavClient(object):
    env_var = 'JUPYTER_YAV_TOKEN'

    def __init__(self, token=None, ssh_key_fallback=True):
        self._client = None
        self._token = token or os.environ.get(self.env_var)
        self._ssh_key_fallback = ssh_key_fallback

    @cached_property
    def client(self):
        if self._token:
            client = Production(authorization=self._token)
        elif is_nirvana():
            raise RuntimeError('missing yav_oauth_token parameter in Run Jupyter notebook operation')
        elif self._ssh_key_fallback:
            client = Production(rsa_auth=HomeSSHRSAAuth())
        else:
            raise RuntimeError(
                'missing YAV Oauth token; pass it via constructor or {} env variable'
                .format(self.env_var)
            )

        return client

    def get_version(self, version, **kwargs):
        return self.client.get_version(version=version, **kwargs)

    def get_secret(self, version, **kwargs):
        return self.get_version(version=version, **kwargs)['value']

    def get_secret_key(self, version, key, **kwargs):
        return self.get_secret(version=version, **kwargs)[key]


_JUPYTER_YAV_CLIENT = JupyterYavClient()
get_secret = _JUPYTER_YAV_CLIENT.get_secret
get_secret_key = _JUPYTER_YAV_CLIENT.get_secret_key
