GTEST()

OWNER(
    g:matrix
)

SRCS(
    tvm_client_ut.cpp
)

PEERDIR(
    alice/matrix/library/clients/tvm_client

    library/cpp/testing/gtest
)

INCLUDE(${ARCADIA_ROOT}/library/recipes/tvmapi/recipe.inc)
INCLUDE(${ARCADIA_ROOT}/library/recipes/tvmtool/recipe.inc)

USE_RECIPE(
    library/recipes/tvmtool/tvmtool
    alice/matrix/library/clients/tvm_client/ut/tvmtool_config.json
)

END()
