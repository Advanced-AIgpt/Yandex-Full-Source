OWNER(
    g:personal-cards
)

PY3TEST()

ENV(YDB_DEFAULT_LOG_LEVEL="DEBUG")
ENV(YDB_ADDITIONAL_LOG_CONFIGS="GRPC_SERVER:DEBUG,TICKET_PARSER:WARN")

DEPENDS(
    alice/personal_cards/bin

    passport/infra/tools/tvmknife/bin
)

PEERDIR(
    alice/personal_cards/integration_tests/lib

    ydb/public/sdk/python
)

TEST_SRCS(
    conftest.py
    test_handlers.py
)

INCLUDE(${ARCADIA_ROOT}/kikimr/public/tools/ydb_recipe/recipe_stable.inc)
INCLUDE(${ARCADIA_ROOT}/library/recipes/tvmapi/recipe.inc)

END()
