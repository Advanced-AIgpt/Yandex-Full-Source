# -*- coding: utf-8 -*-

from __future__ import absolute_import, print_function, division, unicode_literals

import abc
import logging
import os
from threading import RLock

import six

from library.python.vault_client.instances import Production as VaultClient
from library.python.vault_client.errors import ClientError as VaultClientError

from .report import get_global_report, VMReport

SSL_ROOT_CERT_PATHS = (
    '~/.postgresql/root.crt',
    '/etc/ssl/certs/YandexInternalRootCA.pem',
    'root.crt',
    'allCAs.pem',
)

_global_environment = None

logger = logging.getLogger(__name__)

ENVIRONMENTS = {}
DEFAULT_ENVIRONMENT = 'production'
ENV_VAR_PREFIX = 'JC_'


def environment(name, yav_oauth_token=None, **kwargs):
    """
    Функция возвращает объект, отнаследованный от BaseEnvironment.

    С этим объектом можно взаимодействовать явно, а можно использовать
    в качестве context-менеджера.
    В таком случае, во все классы, которые требуют environment при создании
    (EnvironmentDerived), можно будет не передавать явно объект окружения.

    Объект окружения имеет имеет разные property с секретными или не очень данными,
    которые разнятся от окружения к окружению.

    Идея в том, чтобы в одном месте инкапсулировать логику работы с секретницей,
    стандартизировать и максимально упростить получение секретов.

    Если не передать oauth-токен для секретницы, она попытается сама достать
    данные для авторизации из системы (ssh-ключи и проч.)

    """

    if name in ENVIRONMENTS:
        env_class = ENVIRONMENTS[name]
    else:
        raise ValueError('wrong JupyterCloud environment name: {}'.format(name))

    return env_class(
        yav_oauth_token=yav_oauth_token, **kwargs
    )


def get(name):
    if _global_environment is None:
        raise ValueError('no global environment')

    return getattr(_global_environment, name)


def coerce_environment(environment):
    environment = environment or _global_environment

    if environment is None:
        raise ValueError('no explicit or global environment')

    return environment


def secret_property(yav_secret_key, yav_secret_id=None):
    @property
    def _secret_getter(self):
        return self.get_secret(yav_secret_key, yav_secret_id)

    return _secret_getter


class SecretAcquireError(RuntimeError):
    def __init__(self, secret_name, reason):
        self._secret_name = secret_name
        self._reason = reason

    def __str__(self):
        return (
            "Failed to acquire secret {}: {}"
            .format(self._secret_name, self._reason)
        )


class EnvironmentMeta(abc.ABCMeta):
    def __new__(mcs, name, bases=None, dct=None):
        cls = super(EnvironmentMeta, mcs).__new__(mcs, name, bases, dct)

        if name != 'BaseEnvironment':
            assert cls.name and cls.name not in ENVIRONMENTS
            ENVIRONMENTS[cls.name] = cls

        return cls


class BaseEnvironment(six.with_metaclass(EnvironmentMeta, object)):
    __environment_fields = (
        'yav_secret_id',
        'robot_username',
        'sentry_dsn',
        'db_kwargs',
        'qyp_oauth_token',
        'sandbox_oauth_token',
        'staff_oauth_token',
        'id_rsa',
        'ssl_root_cert',
        'salt_api_url',
        'salt_secret',
        'jupyterhub_host',
        'jupyterhub_api_token',
        'vm_prefix',
        'vm_short_prefix',
        'statface_token',
        'redis_password',
        'backend_semaphore'
    )

    yav_secret_id = 'sec-01d83hjj2yehgykzn85n2h5pa5'
    robot_username = 'robot-jupyter-cloud'
    sentr_dsn = 'https://7384c095ac7b4e369b4c9cd3e87610bd@sentry.stat.yandex-team.ru/472'

    qyp_oauth_token = secret_property('qyp_oauth_token')
    id_rsa = secret_property('id_rsa')
    sandbox_oauth_token = secret_property('sandbox-oauth-token')
    staff_oauth_token = secret_property('staff-oauth-token')
    jupyterhub_api_token = secret_property('sandbox_api_token')
    statface_token = secret_property('statface_token')

    def __init__(self, yav_oauth_token=None, **kwargs):
        self._yav_client = VaultClient(
            decode_files=True,
            authorization=yav_oauth_token
        )
        self._cached_secrets = {}

        for arg in kwargs:
            if arg not in self.__environment_fields:
                raise TypeError(
                    "{}.__init__() got an unexpected environment argument '{}'"
                    .format(self.__class__.__name__, arg)
                )

        self.__kwargs_dict = kwargs

        self._cache_lock = RLock()

    def get_secret(self, secret_name, secret_id=None):
        with self._cache_lock:
            if secret_name not in self._cached_secrets:
                logger.debug(
                    'requsting secret %s for environment %s',
                    secret_name, self.__class__.__name__
                )

                try:
                    secret_id = secret_id or self.yav_secret_id
                    last_version = self._yav_client.get_version(secret_id)
                except VaultClientError as e:
                    six.raise_from(SecretAcquireError(secret_name, str(e)), e)

                secrets = last_version['value']

                if secret_name not in secrets:
                    raise SecretAcquireError(secret_name, 'key does not exists in ya vault secret')

                logger.debug('secret %s acquired', secret_name)

                self._cached_secrets[secret_name] = secrets[secret_name]

            return self._cached_secrets[secret_name]

    @abc.abstractproperty
    def name(self):
        pass

    @property
    def db_kwargs(self):
        db_kwargs = dict(
            user='robot_jupyter_cloud',
            port=6432,
            target_session_attrs='read-write',
            sslmode='verify-full',
        )
        ssl_root_cert = self.ssl_root_cert
        if ssl_root_cert:
            db_kwargs['sslrootcert'] = ssl_root_cert

        return db_kwargs

    @property
    def ssl_root_cert(self):
        for path in SSL_ROOT_CERT_PATHS:
            path = os.path.expanduser(path)
            if os.path.exists(path) and os.path.isfile(path):
                return path

        return None

    @property
    def redis_sentinels(self):
        return [
            'man-e4n32zle4p444a1o.db.yandex.net',
            'sas-zykvti0f3q1mml0x.db.yandex.net',
            'vla-za5kqljlvd9cm1yd.db.yandex.net'
        ]

    @abc.abstractproperty
    def jupyterhub_host(self):
        pass

    @abc.abstractproperty
    def vm_prefix(self):
        pass

    @abc.abstractproperty
    def vm_short_prefix(self):
        pass

    @abc.abstractproperty
    def redis_password(self):
        pass

    @abc.abstractproperty
    def backend_semaphore(self):
        pass

    @abc.abstractproperty
    def salt_api_url(self):
        pass

    def __enter__(self):
        global _global_environment
        assert _global_environment is None
        _global_environment = self
        return self

    def __exit__(self, *exc_info):
        global _global_environment
        assert _global_environment is not None
        _global_environment = None

    def __getattribute__(self, name):
        if name.startswith('_'):
            return super(BaseEnvironment, self).__getattribute__(name)

        if name in self.__kwargs_dict:
            return self.__kwargs_dict[name]

        if name in BaseEnvironment.__environment_fields:
            env = os.environ.get(ENV_VAR_PREFIX + name)
            if env is not None:
                return env

        return super(BaseEnvironment, self).__getattribute__(name)

    @classmethod
    def add_cli_arguments(cls, parser, default_environment=DEFAULT_ENVIRONMENT):
        parser.add_argument('--environment', default=default_environment, choices=ENVIRONMENTS)

        group = parser.add_argument_group(
            'Environment', 'arguments for overriding default environment fields'
        )

        for field in cls.__environment_fields:
            group.add_argument('--{}'.format(field.replace('_', '-')))

    @classmethod
    def from_cli_arguments(cls, arguments):
        name = arguments.environment
        env_class = ENVIRONMENTS[name]
        kwargs = {}

        for field in cls.__environment_fields:
            value = getattr(arguments, field, None)
            if value is not None:
                kwargs[field] = value

        return env_class(**kwargs)


class ProductionEnvironment(BaseEnvironment):
    name = 'production'
    jupyterhub_host = 'jupyter.yandex-team.ru'
    vm_prefix = 'jupyter-cloud-'
    vm_short_prefix = 'jc-'
    salt_api_url = 'http://salt-sas.jupyter.yandex-team.ru'
    salt_secret = secret_property('salt_secret', 'sec-01dhkemwckfe8tc5vbk1tps2yq')
    redis_password = secret_property('redis_password', 'sec-01dhkemwckfe8tc5vbk1tps2yq')
    backend_semaphore = 'prod/backend/semaphore'

    @property
    def db_kwargs(self):
        db_kwargs = super(ProductionEnvironment, self).db_kwargs
        db_kwargs.update(
            dbname='jupyterhub',
            host='c-e8515101-dec2-4b85-9c91-5e248bd3b161.rw.db.yandex.net',
            password=self.get_secret('dbaas-production-password'),
        )
        return db_kwargs


class TestingEnvironment(BaseEnvironment):
    name = 'testing'
    jupyterhub_host = 'beta.jupyter.yandex-team.ru'
    vm_prefix = 'testing-jupyter-cloud-'
    vm_short_prefix = 'tjc-'
    salt_api_url = 'http://salt-sas.beta.jupyter.yandex-team.ru'
    salt_secret = secret_property('salt_secret', 'sec-01dh6emwya97r6z2w8pc88m7a2')
    redis_password = secret_property('redis_password', 'sec-01dh6emwya97r6z2w8pc88m7a2')
    backend_semaphore = 'test/backend/semaphore'

    @property
    def db_kwargs(self):
        db_kwargs = super(TestingEnvironment, self).db_kwargs
        db_kwargs.update(
            dbname='jupyterhub_test',
            host='c-726876b0-fb04-49ca-80b0-0ffecdd7a687.rw.db.yandex.net',
            password=self.get_secret('dbaas-testing-password'),
        )
        return db_kwargs


class EnvironmentDerived(object):
    def __init__(self, environment=None):
        self._environment = environment

    @property
    def environment(self):
        return coerce_environment(self._environment)

    @property
    def report(self):
        # to not to fail without global installed report in old scripts
        return get_global_report(assert_exists=False) or VMReport()


add_cli_arguments = BaseEnvironment.add_cli_arguments
from_cli_arguments = BaseEnvironment.from_cli_arguments
