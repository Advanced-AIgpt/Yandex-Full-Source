#!/usr/bin/env python
# encoding: utf-8
import json
import sys
import time
from operator import itemgetter

import requests

from auth import make_auth_headers


class StatDict(object):
    _url = 'https://upload.stat.yandex-team.ru/_api/dictionary'

    def __init__(self, name, cache_ttl=60*60, username=None, password=None, token=None):
        """
        Загрузка словаря из stat
        :param basestring name: Название словаря
        :param int cache_ttl: Сколько секунд хранить кэшированный результат
        :param str|None username: Имя робота на стате.
            Если не задано, берётся из переменной окружения `STATFACE_USERNAME`, или из файла `~/.statbox/statface_username`
        :param str|None password: Пароль робота на стате.
            Если не задан, берётся из переменной окружения `STATFACE_PASSWORD`, или из файла `~/.statbox/statface_password`
        """
        self.name = name
        self._auth_headers = make_auth_headers(username, password, token)
        self._cache_ttl = cache_ttl
        self._next_load = 0
        self._content = {}
        self.last_err = None

    def load(self):
        try:
            response = requests.get(url=self._url, headers=self._auth_headers, params={'name': self.name}, timeout=3)
            return json.loads(response.content)
        except Exception, err:
            msg = "Can't load stat dict '%s': %s" % (self.name, err)
            sys.stderr.write(msg)
            self.last_err = msg
            return None

    @property
    def content(self):
        """Содержимое словаря"""
        if self._next_load < time.time():
            value = self.load()
            if value is not None:
                for k, v in value.iteritems():
                    if not v:
                        value[k] = k  # Подставляем ключи на место отсутствующих значений
                self._content = value
                self._next_load = time.time() + self._cache_ttl
        return self._content

    def as_list(self, sort_by='value'):
        """Возвращает словарь как список, сортируя либо по ключам, либо по значениям"""
        items = ({'key': k, 'value': v}
                 for k, v in self.content.iteritems())
        return sorted(items, key=itemgetter(sort_by))
