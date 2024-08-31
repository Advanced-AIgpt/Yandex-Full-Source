PY3TEST()

STYLE_PYTHON()

OWNER(g:voicetech-infra)

PEERDIR(
    alice/cuttlefish/library/protos
    alice/cuttlefish/library/python/apphost_grpc_client
    alice/cuttlefish/library/python/testing
    alice/cuttlefish/tests/common
    contrib/python/pytest-asyncio
    contrib/python/tornado/tornado-6
)

DEPENDS(
    alice/cuttlefish/bin/asr_adapter
    voicetech/tools/evlogdump
    alice/rtlog/evlogdump
)

TEST_SRCS(
    asr_adapter.py
    test_recognize.py
)

DATA(arcadia/alice/cuttlefish/package/asr_adapter)

END()
