#!/usr/bin/env python
# encoding: utf-8
"""
Disclaimer: Чтобы функции в этом модуле работали с нашим кластером,
нужно прописать пароль либо в переменную окружения `CH_VIEWER_PASSWD`, либо в файл `~/.dbaas/viewer_passwd`
"""
import json

import requests

from utils.auth import CERT_PATH
from voicelogs import make_headers, HOSTS
from time import sleep
import logging


def retries(max_tries, delay=1, backoff=2, exceptions=(Exception,), hook=None, log=True, raise_class=None):
    """
        Wraps function into subsequent attempts with increasing delay between attempts.
        Adopted from https://wiki.python.org/moin/PythonDecoratorLibrary#Another_Retrying_Decorator
        (copy-paste from arcadia sandbox-tasks/projects/common/decorators.py)
    """
    def dec(func):
        def f2(*args, **kwargs):
            current_delay = delay
            for n_try in xrange(0, max_tries + 1):
                try:
                    return func(*args, **kwargs)
                except exceptions as e:
                    if n_try < max_tries:
                        if log:
                            logging.error(
                                "Error in function %s on %s try:\n%s\nWill sleep for %s seconds...",
                                func.__name__, n_try, e, current_delay
                            )
                        if hook is not None:
                            hook(n_try, e, current_delay)
                        sleep(current_delay)
                        current_delay *= backoff
                    else:
                        logging.error("Max retry limit %s reached, giving up with error:\n%s", n_try, e)
                        if raise_class is None:
                            raise
                        else:
                            raise raise_class("Max retry limit {} reached, giving up with error: {}".format(
                                n_try,
                                str(e))
                            )

        return f2
    return dec


@retries(5, 10, backoff=4)
def post(url, data, headers, verify):
    return requests.post(url, data=data, headers=headers, verify=verify)


def post_req(query, host=HOSTS[0], headers=None, db='analytics', options=None, as_json=False):
    """
    Выполнить произвольный запрос к кликхаусу
    :param str|unicode query: Текст запроса
    :param dict[str,str]|None headers: Заголовки запроса.
        Главным образом нужны для аутентификации.
    :param str db:
    :param None|dict[str, int|str] options: Сессионные переменные Кликхауса
    :param bool as_json: Нужно ли добавить к запросу "FORMAT JSON" и попарсить ответ как json.
    :return: Строка в формате tsv. В случае проблем с запросом, будет выброшено исключение с дебаг-инфой.
    """
    url = 'https://%s:8443/?database=%s' % (host, db)
    if options:
        url = '%s&%s' % (url, '&'.join('%s=%s' % opt for opt in options.iteritems()))

    if isinstance(query, unicode):
        query = query.encode('utf-8')

    if as_json:
        query += '\nFORMAT JSON'

    response = post(
        url,
        data=query,
        headers=headers or make_headers(),
        verify=CERT_PATH
    )
    if response.status_code == 200:
        if as_json:
            return json.loads(response.content)
        else:
            return response.content
    else:
        raise UserWarning('Request Error: %s, %s' % (response.status_code, response.content))


def insert_req(table, values, host=HOSTS[0], headers=None, db='analytics'):
    """
    Аналогичен `post_req`, но вместо произвольного запроса делает INSERT в таблицу `table`.
    В качестве values нужно передавать
    :param str table: Название таблицы
    :param str|Iterator[str] values: Строка в кликхаусовом tsv-формате, в utf-8.
        https://clickhouse.yandex/docs/ru/formats/tabseparated/
        Может быть передана по кусочкам с помощью итератора.
    :param str host:
    :param dict[str, str]|None headers:
    :param str db:
    :return: Ответ от сервера. В нормальном случае, пустой.
        В случае проблем, выбрасывает исключение с дебаг-информацией.
    """
    # values - already tab separated
    url = 'https://%s:8443/?database=%s&query=INSERT INTO %s FORMAT TabSeparated'
    response = post(
        url % (host, db, table),
        data=values,
        headers=headers or make_headers(),
        verify=CERT_PATH,
    )
    if response.status_code == 200:
        return response.content
    else:
        raise UserWarning('Insertion Error: %s, %s' % (response.status_code, response.content))

