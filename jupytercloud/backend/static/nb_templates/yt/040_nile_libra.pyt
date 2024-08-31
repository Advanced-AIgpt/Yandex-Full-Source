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
#     name_en: "Nile: Libra, Baobab, Tamus"
#     description_ru: >-
#       Работа с user-sessions: библиотека Libra и Tamus.
#     description_en: >-
#       Use Libra and Tamus in Nile.
#       Work with UserSessions.
# ---

# %% [markdown]
# Перед самым первым запуском не забудьте получить [YT токен](https://yt.yandex-team.ru/docs/gettingstarted.html#auth)

# %%
from nile.api.v1 import (
    Record,
    clusters,
    extractors as ne,
    filters as nf,
    aggregators as na,
    files as nfl
)
from qb2.api.v1 import (
    extractors as qe,
    filters as qf
)

cluster = clusters.YT("{{ cluster }}")

# %% [markdown] run_control={"marked": false}
# # Nile: Libra и Tamus
#
# [Документация](https://logos.yandex-team.ru/statbox/nile/index.html)
#
# [Поддержка telegram](https://t.me/joinchat/EdSjYEKw7747LXqboecwlA)
#
# [Libra](https://wiki.yandex-team.ru/logs/man/libra)
#
# [Tamus](https://wiki.yandex-team.ru/baobab/tamus)
#
# Шаблон написан и актуален в апреле 2020.

# %%
def process_user_sessions(groups):
    import libra
    import baobab
    import tamus

    for key, records in groups:
        try:
            session = libra.ParseSession(records, './blockstat.dict')
        except RuntimeError:  # Fat user
            continue
        except Exception as e:
            e.args = e.args + ('Failed on key {}'.format(key.key), )
            raise e

        for req in session:
            if req.IsA('TBaobabProperties'):
                joiner = req.BaobabTree()
                if joiner:
                    markers = tamus.check_rules_merged(
                        {'mymarker': '#wizard_result [@wizard_name = "companies"]'},
                        joiner
                    )
                    for block in markers.get_blocks("mymarker"):
                        clicks = 0
                        for event in joiner.get_events_by_block_subtree(block):
                            if isinstance(event, baobab.common.Click):
                                clicks += 1
                        yield Record(
                            reqid=req.ReqID,
                            block_name=block.name,
                            shows=1,
                            clicks=clicks)

# %%
job = cluster.job()

stream = job.table(
    # тут должны быть user-sessions
    "{{ path }}"
).groupby(
    'key'
).sort(
    'subkey'
).reduce(
    process_user_sessions,
    files=[
        nfl.StatboxDict('blockstat.dict', use_latest=True),
    ],
    memory_limit=6144
).groupby(
    'block_name'
).aggregate(
    shows=na.sum('shows'),
    clicks=na.sum('clicks'),
    reqids=na.count(),
).put(
    '//tmp/{{ username }}/result', schema=dict(block_name=str, shows=int, reqids=int, clicks=int)
)

# %%
job.run()

# %%
records = cluster.read('//tmp/{{ username }}/result')
dataframe = records.as_dataframe()
dataframe
