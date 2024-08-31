# ---
# jupyter:
#   jupytext:
#     text_representation:
#       extension: .py
#       format_name: percent
#       format_version: '1.3'
#       jupytext_version: 1.4.1
#   kernelspec:
#     display_name: Arcadia Python 3 Default
#     language: python
#     name: arcadia_default_py3
#   nb_template:
#     name_en: Pandas
#     description_en: >-
#       Скачать данные из YT локально в Pandas dataframe.
#
#       <p class="text-warning">Следите за оперативной памятью!
#       Таблицы более ~200 MB нельзя обработать таким шаблоном.</p>
# ---

# %% [markdown]
Перед самым первым запуском не забудьте получить [YT токен](https://yt.yandex-team.ru/docs/gettingstarted.html#auth).

# %%
from nile.api.v1 import clusters

path = "{{ path }}"

# %%
cluster = clusters.YT("{{ cluster }}")

records = cluster.read(path)
dataframe = records.as_dataframe()

dataframe
