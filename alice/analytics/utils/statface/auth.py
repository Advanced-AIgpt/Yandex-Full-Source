#!/usr/bin/env python
# encoding: utf-8
from utils.auth import choose_credential, CredentialNotFound


def choose_robot_credentials(username, password, token):
    try:
        token = choose_credential(token, 'STATFACE_TOKEN', '~/.statbox/statface_token')
    except CredentialNotFound:
        pass  # Это норм. Попробуем по пользователю+паролю
    else:
        return username, password, token

    return (
        choose_credential(username, 'STATFACE_USERNAME', '~/.statbox/statface_username'),
        choose_credential(password, 'STATFACE_PASSWORD', '~/.statbox/statface_password'),
        None,
    )


def make_auth_headers(username, password, token):
    username, password, token = choose_robot_credentials(username, password, token)
    if token is None:
        return {'StatRobotAuth': '{}:{}'.format(username, password)}
    else:
        return {"Authorization": "OAuth {}".format(token)}
