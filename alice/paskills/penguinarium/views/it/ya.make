PY3TEST()

OWNER(
    g:paskills
    penguin-diver
)

SIZE(MEDIUM)

TEST_SRCS(
    conftest.py
    test_views.py
)

PEERDIR(
    alice/paskills/penguinarium

    contrib/python/asynctest
    contrib/python/fakeredis
    contrib/python/pytest-asyncio
)

DATA(
    # qe_model
    sbr://1322972048
)

ENV(YDB_YQL_SYNTAX_VERSION="0")
INCLUDE(${ARCADIA_ROOT}/kikimr/public/tools/ydb_recipe/recipe_stable.inc)

REQUIREMENTS(ram:32)

END()
