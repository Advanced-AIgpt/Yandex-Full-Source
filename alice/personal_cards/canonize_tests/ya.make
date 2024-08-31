EXECTEST()

OWNER(
    g:personal-cards
)

RUN(
    NAME IntegrationTests
    alice-personal_cards-integration_tests --test-param dump_file_path=${TEST_OUT_ROOT}/integration_tests_dump.log
    CANONIZE_LOCALLY ${TEST_OUT_ROOT}/integration_tests_dump.log
)

DEPENDS(
    alice/personal_cards/bin
    alice/personal_cards/integration_tests
    passport/infra/tools/tvmknife/bin
)

SIZE(SMALL)

INCLUDE(${ARCADIA_ROOT}/kikimr/public/tools/ydb_recipe/recipe_stable.inc)
INCLUDE(${ARCADIA_ROOT}/library/recipes/tvmapi/recipe.inc)

END()
