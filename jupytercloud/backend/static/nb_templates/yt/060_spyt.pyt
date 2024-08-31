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
#     display_name: Python 3
#     language: python
#     name: python3
#   nb_template:
#     name_en: "Spark over YT"
#     description_ru: >-
#       Базовый шаблон Spark over YT, подключение к кластеру Spark и чтение датафрейма.
# ---

# %% [markdown]
# Перед тем, как использовать **Spark** в **Jupyter**, нужно поднять кластер для своей команды по [инструкции](https://wiki.yandex-team.ru/spyt/cluster/).
# Если кластер у вашей команды уже есть, то вы должны знать его *discovery_path*. Спросите его у того, кто запускал кластер.
#
# Перед первым запуском нужно положить [YT Token](https://yt.yandex-team.ru/docs/gettingstarted.html#auth) в файл ~/.yt/token
#
# Полезные ссылки:
# - [Чат поддержки](https://t.me/joinchat/BYmS6xFq2ob5xmrbdbW66w)
# - [Примеры на Github](https://wiki.yandex-team.ru/spyt/jupyter/#primerykoda)
# - [Официальная документация Apache Spark](https://spark.apache.org/docs/latest/)

# %% [markdown]
# В следующей ячейке настраиваются координаты доступа к кластеру Spark.
#
# Раскомментируйте ячейку, если кластер еще не настроен:

# %%
# %%writefile ~/spyt.yaml
# yt_proxy: "{{ cluster }}"
# discovery_path: "<DISCOVERY_PATH>"

# %%
import spyt
import pyspark.sql.functions as F
import pyspark.sql.types as T
from pyspark.sql.functions import col, lit, broadcast

# %%
spark = spyt.connect()
spyt.info(spark)

# %%
df = spark.read.yt("{{ path }}")

# %%
df.show()

# %%
df.count()
