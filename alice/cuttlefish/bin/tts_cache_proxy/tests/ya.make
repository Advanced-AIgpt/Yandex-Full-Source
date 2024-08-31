PY3TEST()

STYLE_PYTHON()

OWNER(g:voicetech-infra)

PEERDIR(
    alice/cuttlefish/library/protos
    alice/cuttlefish/library/python/apphost_grpc_client
    alice/cuttlefish/library/python/testing
    alice/cachalot/tests/integration_tests/lib
    alice/cuttlefish/tests/common
    alice/uniproxy/library/protos
    contrib/python/asynctest
    contrib/python/pytest-asyncio
    contrib/python/tornado/tornado-4
    library/python/codecs
    apphost/lib/grpc/protos
    apphost/lib/proto_answers
)

DEPENDS(
    alice/cachalot/bin
    alice/cuttlefish/bin/tts_cache_proxy
    alice/rtlog/evlogdump
    voicetech/tools/evlogdump
)

INCLUDE(${ARCADIA_ROOT}/kikimr/public/tools/ydb_recipe/recipe_stable.inc)

INCLUDE(${ARCADIA_ROOT}/library/python/redis_utils/recipe/recipe.inc)

TEST_SRCS(
    # Daemons
    tts_cache_proxy.py
    # Tests
    test_tts_cache_proxy.py
)

# Cachalot fixture is too heavy (it uses ydb + redis + cachalot itself)
# So we need this requirements
SIZE(MEDIUM)

REQUIREMENTS(cpu:4)

END()
