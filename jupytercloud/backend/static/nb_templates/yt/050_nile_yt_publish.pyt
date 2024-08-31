# -*- coding: utf-8 -*-
# ---
# jupyter:
#   jupytext:
#     text_representation:
#       extension: .py
#       format_name: percent
#       format_version: '1.3'
#       jupytext_version: 1.4.1
#   kernelspec:
#     display_name: Arcadia Python 2 Default
#     language: python
#     name: arcadia_default_py2
#   nb_template:
#     name_ru: "Nile: публикация на Statface"
#     name_en: "Nile: Statface publish"
#     description_ru: >-
#       Публикация рассчета на Nile на stat.yandex-team.ru.
#     description_en: >-
#       Publish form Nile to stat.yandex-team.ru.
# ---

# %% [markdown]
# Перед самым первым запуском не забудьте получить [YT токен](https://yt.yandex-team.ru/docs/gettingstarted.html#auth)

# %%
from nile.api.v1 import (
    clusters,
    extractors as ne,
    filters as nf,
    aggregators as na,
    statface as ns,
)
from qb2.api.v1 import (
    extractors as qe,
    filters as qf
)

cluster = clusters.yt.Hahn()

# %% [markdown] run_control={"marked": false}
# # Nile: публикация на Стат, YT драйвер
#
# [Документация](https://logos.yandex-team.ru/statbox/nile/index.html)
#
# [Поддержка telegram](https://t.me/joinchat/EdSjYEKw7747LXqboecwlA)
#
# [Туториал по публикации](https://logos.yandex-team.ru/statbox/nile/advanced_tutorial/publish.html)

# %% [markdown]
# Не забудьте положить токен для Stat в [.statbox/statface_auth.yaml](https://wiki.yandex-team.ru/statbox/statface/externalreports/statface-python-client/#polucheniedostupov)

# %%
STATFACE_REPORT = ns.StatfaceReport().title(
    'Название отчёта'
).dimensions(
    ns.Date('fielddate')
).measures(
    ns.Number('records').title('Записей')
).path(
    'Adhoc/Adhoc/{{ username }}/TestReport'
).replace_mask(
    'fielddate'
)

statface_publisher = STATFACE_REPORT.scale(
    'daily'
).client(ns.client.StatfaceProductionClient())

# %%
job = cluster.job()
stream = job.table(
    '{{ path }}'
).groupby(
    'fielddate'
).aggregate(
    records=na.count()
).put(
    '//tmp/result', schema=dict(fielddate=str, records=int)
).publish(
    statface_publisher
)

# %%
job.run()
