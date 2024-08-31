PY3TEST()

STYLE_PYTHON()

OWNER(g:voicetech-infra)

PEERDIR(
    alice/cuttlefish/library/protos
    alice/cuttlefish/library/python/apphost_grpc_client
    alice/cuttlefish/library/python/testing
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
    alice/cuttlefish/bin/tts_adapter
    alice/rtlog/evlogdump
    voicetech/tools/evlogdump
)

TEST_SRCS(
    # Daemons
    tts_adapter.py
    # Tests
    test_tts_adapter.py
)

END()