# coding: utf-8
from __future__ import unicode_literals, absolute_import

import logging
import os
from types import NoneType

import yenv as yenv_lib

from vins_core.utils.strings import smart_unicode

logger = logging.getLogger(__name__)


DEFAULT_VINS_PREFIX = 'VINS_'


class Settings(object):
    _all_settings = {}

    @classmethod
    def get(cls, name, default=None, yenv=None, prefix=DEFAULT_VINS_PREFIX):
        if yenv and not isinstance(yenv, dict):
            raise ValueError('"yenv" parameter must be dict-object')

        env_name = '{0}{1}'.format(prefix, name)

        if env_name in os.environ:
            cls._all_settings[name] = os.environ[env_name]
        elif yenv:
            cls._all_settings[name] = yenv_lib.choose_key_by_type(yenv, fallback=True)
        elif default is not None:
            cls._all_settings[name] = default
        else:
            raise ValueError('No value for "%s"' % name)

        return cls._all_settings[name]

    @classmethod
    def get_bool(cls, name, default=False, yenv=None, prefix=DEFAULT_VINS_PREFIX):
        value = get_setting(name, default=default, yenv=yenv, prefix=prefix)
        if isinstance(value, (int, long, bool, NoneType)):
            return bool(value)
        elif isinstance(value, basestring):
            value = smart_unicode(value).lower()
            if value in ['1', 'true', 'yes', 'da']:
                return True
            if value in ['0', 'false', 'no', 'net']:
                return False

        raise ValueError('Cannot interpret %s as bool for variable %s' % (value, name))


get_setting = Settings.get
get_bool_setting = Settings.get_bool
