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
#     name_ru: "Nile: простая агрегация"
#     name_en: "Nile: simple aggregate"
#     description_ru: >-
#       Nile шаблон с простой агрегацей по колонке
#     description_en: >-
#       Simple aggregate using Nile
# ---

# %% [markdown]
# Перед самым первым запуском не забудьте получить [YT токен](https://yt.yandex-team.ru/docs/gettingstarted.html#auth)

# %%
from nile.api.v1 import (
    clusters,
    extractors as ne,
    filters as nf,
    aggregators as na
)
from qb2.api.v1 import (
    extractors as qe,
    filters as qf
)

cluster = clusters.YT("{{ cluster }}")

# %% [markdown] run_control={"marked": false}
# # Nile: простая агрегация, YT драйвер
#
# [Документация](https://logos.yandex-team.ru/statbox/nile/index.html)
#
# [Поддержка telegram](https://t.me/joinchat/EdSjYEKw7747LXqboecwlA)

# %%
job = cluster.job()
stream = job.table(
    "{{ path }}"
).filter(
    ...
).project(
    ne.all() # Все колонки
).groupby(
    ...
).aggregate(
    records=na.count(),
).put(
    '//tmp/{{ username }}/result', # schema={...}
)

# %%
job.run()

# %%
records = cluster.read('//tmp/{{ username }}/result')
dataframe = records.as_dataframe()
dataframe
