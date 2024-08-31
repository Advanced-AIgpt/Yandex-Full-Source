#!/usr/bin/env python
# encoding: utf-8
from nile.api.v1.statface import StatfaceProductionClient, StatfaceBetaClient

from auth import choose_robot_credentials


def make_stat_client(proxy='prod', username=None, password=None, token=None):
    """
    Создание клиента для публикации данных из nile в statface
    :param str proxy: "beta" или "prod"
    :param str|None username: Имя робота на стате.
        Если не задано, берётся из переменной окружения `STATFACE_USERNAME`, или из файла `~/.statbox/statface_username`
    :param str|None password: Пароль робота на стате.
        Если не задан, берётся из переменной окружения `STATFACE_PASSWORD`, или из файла `~/.statbox/statface_password`
    :param str|None token: Токен для аутентификации на стате. Является более предпочтительным методом.
        Если не задан, берётся из переменной окружения `STATFACE_TOKEN`, или из файла `~/.statbox/statface_token`
    :rtype: StatfaceBetaClient|StatfaceProductionClient
    """
    username, password, token = choose_robot_credentials(username, password, token)

    if proxy == 'beta':
        return StatfaceBetaClient(username, password, token)
    elif proxy == 'prod':
        return StatfaceProductionClient(username, password, token)
    else:
        raise UserWarning('unknown proxy "%s", must be "beta" or "prod"' % proxy)

