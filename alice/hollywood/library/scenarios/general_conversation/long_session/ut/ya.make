UNITTEST_FOR(alice/hollywood/library/scenarios/general_conversation/long_session)

ENV(YDB_YQL_SYNTAX_VERSION="0")
INCLUDE(${ARCADIA_ROOT}/kikimr/public/tools/ydb_recipe/recipe_stable.inc)

OWNER(
    deemonasd
    g:hollywood
    g:alice_boltalka
)

PEERDIR(
    alice/bass/libs/ydb_helpers

    library/cpp/testing/unittest
)

SRCS(
    long_session_client_ut.cpp
)

END()
