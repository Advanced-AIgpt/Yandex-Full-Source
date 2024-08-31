#!/usr/bin/env python
# encoding: utf-8
import os
from os.path import realpath, join


CERT_PATH = realpath(join(__file__, '../allCAs.pem'))  # Скачать можно здесь "https://crls.yandex.net/allCAs.pem"


class CredentialNotFound(Exception):
    pass


def choose_credential(value, envvar, path):
    """
    Выбирает аутентификационные данные, в порядке приоритета:
         либо явно заданные,
         либо из переменной окружения,
         либо из заданного файла
    :param str value: Заданное пользователем значение
    :param str envvar: Переменная окружения, в которую может быть записано значение
    :param str path: Путь, в котором может лежать значение
    :rtype: str
    :return: Выбранное значение
    """
    if value is None:
        value = os.getenv(envvar)  # В нирвана операциях используется так

    path = os.path.expanduser(path)
    if value is None and os.path.exists(path):
        value = open(path).read().strip()  # Можно использовать для локальных тестов

    if value is None:
        msg = 'credential not found in "{}" nor "{}"'.format(envvar, path)
        raise CredentialNotFound(msg)
    else:
        return value


def make_oauth_header(token, envvar, path, content_type="application/json; charset=utf-8"):
    # Словарь хэдеров, для аутентификации через http oauth
    token = choose_credential(token, envvar, path)
    return {"Content-Type": content_type,
            "Authorization": "OAuth {}".format(token)}
