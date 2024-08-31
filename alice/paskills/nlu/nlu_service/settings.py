# coding: utf-8
from __future__ import unicode_literals

import logging
import os


logger = logging.getLogger(__name__)


PREFIX = 'NER_'
BOOL_TRUE = ['1', 'true']
BOOL_FALSE = ['0', 'false']


class ConfigurationError(Exception):
    pass


class Settings(object):
    _all_settings = {}

    @classmethod
    def get_str_setting(cls, name, default=None):
        if name in cls._all_settings:
            return cls._all_settings[name]
        else:
            environ_key = PREFIX + name
            logger.info(environ_key)
            value = str(os.environ.get(environ_key, default))
            if value is None:
                raise ConfigurationError('Missing environment variable %s' % environ_key)
            cls._all_settings[name] = value
            return value

    @classmethod
    def get_bool_setting(cls, name, default=None):
        if name in cls._all_settings:
            return cls._all_settings[name]
        else:
            string_value = cls.get_str_setting(name, default).lower()
            if string_value in BOOL_TRUE:
                cls._all_settings[name] = True
            elif string_value in BOOL_FALSE:
                cls._all_settings[name] = False
            else:
                raise ConfigurationError('Invalid value for boolean setting: "%s"' % string_value)
            return cls._all_settings[name]

    @classmethod
    def _get_number_setting(cls, name, converter_function, default=None):
        if name in cls._all_settings:
            return cls._all_settings[name]
        else:
            string_value = cls.get_str_setting(name, default)
            try:
                value = converter_function(string_value)
            except ValueError:
                raise ConfigurationError('Invalid value for %s setting: "%s"' % (converter_function, string_value))
            cls._all_settings[name] = value
            return cls._all_settings[name]

    @classmethod
    def get_int_setting(cls, name, default=None):
        return cls._get_number_setting(name, int, default)

    @classmethod
    def get_float_setting(cls, name, default=None):
        return cls._get_number_setting(name, float, default)


WIZARD_URL = Settings.get_str_setting('WIZARD_URL', 'http://hamzard.yandex.net:8891/wizard')
WIZARD_TIMEOUT = Settings.get_float_setting('WIZARD_TIMEOUT', 0.15)
WIZARD_MAX_RETRIES = Settings.get_int_setting('WIZARD_MAX_RETRIES', 2)

LOG_NER_ENTITIES = Settings.get_bool_setting('LOG_NER_ENTITIES', True)

API_WAIT_BEFORE_SHUTDOWN = Settings.get_float_setting('API_WAIT_BEFORE_SHUTDOWN', 2.)

REVERSE_NORMALIZER_RESOURCE_DIR = Settings.get_str_setting(
    'NORMALIZER_RESOURCE_DIR',
    '/app/binary/reverse-normalizer/ru',
)
