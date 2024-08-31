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
#     default_weight: 1
#     name_ru: "Nile: пустой поток"
#     name_en: "Nile: empty stream"
#     description_ru: >-
#       Базовый Nile шаблон, без запуска и итогового put.
#     description_en: >-
#       Empty Nile template.
#       Only stream init.
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
# # Nile: пустой поток, YT драйвер
#
# [Документация](https://logos.yandex-team.ru/statbox/nile/index.html)
#
# [Поддержка telegram](https://t.me/joinchat/EdSjYEKw7747LXqboecwlA)

# %%
job = cluster.job()
stream = job.table("{{ path }}")  # Don't forget .put() !

# %%
job.run()
