#!/usr/bin/env python
# encoding: utf-8
"""
Парсинг конфигов
"""
from os.path import realpath, dirname, join, exists

from utils.serializers import yaml  # yaml с OrderedDict


class Conf(object):
    ROOT_PATH = dirname(dirname(realpath(__file__)))

    CONF_TYPE = None  # Для перезаписи в подклассах. Соответствует имени директории с конфигами

    def __init__(self, key):
        self.key = key
        self._conf = None

    def get_conf_dir(self):
        return join(self.ROOT_PATH, self.CONF_TYPE, self.key)

    def get_conf(self):
        if self._conf is None:
            with open(join(self.get_conf_dir(), 'conf.yaml')) as inp:
                self._conf = yaml.load(inp)
        return self._conf

    def get_spec_content(self, filename, as_unicode=True):
        own_path = join(self.get_conf_dir(), 'spec', filename)
        if exists(own_path):
            return read_spec(own_path, as_unicode)
        else:
            return get_common_spec(filename, as_unicode)

    def save_conf_on_disk(self):
        with open(join(self.get_conf_dir(), 'conf.yaml'), 'w') as out:
            yaml.dump(self._conf, out)

    def __repr__(self):
        return '<{}: {}>'.format(self.__class__.__name__, self.key)


def get_common_spec(filename, as_unicode=True):
    path = join(Conf.ROOT_PATH, 'common_spec', filename)
    if exists(path):
        return read_spec(path, as_unicode)
    return None


def read_spec(path, as_unicode=True):
    with open(path) as inp:
        if as_unicode:
            return unicode(inp.read(), 'utf-8')
        else:
            return inp.read()
