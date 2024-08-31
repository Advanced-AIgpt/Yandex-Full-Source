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
    voicetech/library/proto_api
)

DEPENDS(
    alice/cuttlefish/bin/yabio_adapter
    voicetech/tools/evlogdump
    alice/rtlog/evlogdump
)

TEST_SRCS(
    yabio_adapter.py
    test.py
)

DATA(arcadia/alice/cuttlefish/package/yabio_adapter)

REQUIREMENTS(ram:13)

END()
