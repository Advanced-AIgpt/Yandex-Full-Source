# -*- coding: utf-8 -*-

from __future__ import absolute_import, print_function, division, unicode_literals

import logging

from functools import cached_property

from traitlets.config import get_config

from jupytercloud.backend.lib.clients.qyp import QYPClient

from . import db
from .environment import EnvironmentDerived


logger = logging.getLogger(__name__)


class JupyterCloud(EnvironmentDerived):
    @cached_property
    def qyp(self):
        c = get_config()
        c.QYPClient.vm_name_prefix = self.environment.vm_prefix
        c.QYPClient.vm_short_name_prefix = self.environment.vm_short_prefix
        c.QYPClient.oauth_token = self.environment.qyp_oauth_token
        return QYPClient(config=c)

    def get_users(self):
        result = db.execute('SELECT name FROM users;', environment=self._environment)

        return [line['name'] for line in result]

    def get_active_users(self):
        result = db.execute("""
            SELECT
                u.name
            FROM
                spawners AS s
            INNER JOIN
                users AS u
            ON
                s.user_id = u.id
            WHERE
                s.server_id IS NOT NULL;
        """, environment=self._environment)

        return [line["name"] for line in result]

    def get_user_host(self, user):
        base_url = '/user/{}/'.format(user)

        result = db.execute(
            "SELECT ip FROM servers WHERE base_url='{}' AND cookie_name='cookie';"
            .format(base_url),
            environment=self._environment
        )

        if not result:
            raise ValueError('failed to get host for user {}'.format(user))
        if len(result) > 1:
            raise ValueError('fetched {} hosts for user {}'.format(len(result), user))

        return result[0]['ip']

    def get_users_hosts(self, users=None):
        if users:
            users = set(users)

        result = db.execute(
            "SELECT base_url, ip FROM servers WHERE cookie_name='cookie'",
            environment=self._environment,
        )
        users_hosts = {}

        for line in result:
            base_url = line['base_url']
            parts = base_url.strip('/').split('/')
            if len(parts) != 2 and parts[0] != 'user':
                raise ValueError('bad base url {}'.format(base_url))

            user = parts[1]
            host = line['ip']

            if users and user not in users or not host:
                continue

            if user in users_hosts:
                raise ValueError('fetched more than one host for user {}'.format(user))

            users_hosts[user] = host

        if users:
            missing_users = users - set(users_hosts)
            if missing_users:
                raise ValueError('failed to get hosts for users: {}'
                                 .format(', '.join(missing_users)))

        return users_hosts
