#!/usr/bin/env python
# encoding: utf-8
from __future__ import unicode_literals

from utils.nirvana.op_caller import call_as_operation
from utils.toloka.std_pool import StdPool


# Используется в "Run Toloka Std Pool"
# https://nirvana.yandex-team.ru/operation/e4ba3f74-69f2-469e-be60-99bfc80335af/subprocess

def create_pool(**kwargs):
    """
    Значения опций смотреть в StdPool
    Типичный вход:
    {
      "random_accuracy": 0.5,
      "skill_id": "2753",
      "skill_url": "https://sandbox.toloka.yandex.ru/requester/quality/skill/2753",
      "overlap": 5,
      "priority": 50,
      "is_sandbox": True,
      "prj_url": "https://sandbox.toloka.yandex.ru/requester/project/11649",
      "prj_id": "11649",
      "page_size": 22
    }
    """
    # Удаляем информационные поля, которые могли быть скопированы вместе с полезными
    kwargs.pop('skill_url', None)
    kwargs.pop('prj_url', None)
    kwargs.pop('prj_name_and_instructions_hash', None)

    pool = StdPool(**kwargs)
    pool.create_pool()

    return [{  # Формат, совместимый с Hitman-кубиками
        "poolId": pool.pool_id,
        "projectId": pool.prj_id,
        "environment": pool.get_env_name(),
    }]


if __name__ == '__main__':
    call_as_operation(create_pool)

