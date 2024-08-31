PY3TEST()
OWNER(g:voicetech-infra)

PEERDIR(
    alice/cachalot/tests/integration_tests/lib
    alice/cachalot/tests/test_cases
)

DEPENDS(
    alice/cachalot/bin
)

ENV(YDB_USE_IN_MEMORY_PDISKS=true)
INCLUDE(${ARCADIA_ROOT}/kikimr/public/tools/ydb_recipe/recipe_stable.inc)

TEST_SRCS(
    test_all.py
)

SIZE(MEDIUM)

REQUIREMENTS(
    cpu:4
)

END()
