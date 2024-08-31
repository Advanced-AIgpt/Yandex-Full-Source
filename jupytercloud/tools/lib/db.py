# -*- coding: utf-8 -*-

from __future__ import absolute_import, print_function, division, unicode_literals

import contextlib
import psycopg2

from psycopg2.extras import DictCursor

from .environment import coerce_environment


@contextlib.contextmanager
def make_cursor(environment=None, commit=False):
    environment = coerce_environment(environment)

    connect_kwargs = environment.db_kwargs

    with psycopg2.connect(**connect_kwargs) as connection:
        with connection.cursor(cursor_factory=DictCursor) as cursor:
            yield cursor


def execute(query, environment=None):
    with make_cursor(environment) as cursor:
        cursor.execute(query)
        return cursor.fetchall()
