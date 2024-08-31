PY3TEST()

STYLE_PYTHON()

OWNER(g:voicetech-infra)

PEERDIR(
    contrib/python/pytest-asyncio
    contrib/python/tornado/tornado-6
    apphost/api/service/python
    alice/cuttlefish/library/python/apphost_grpc_client
    alice/cuttlefish/library/python/apphost_grpc_client/ut_protos
)

TEST_SRCS(
    apphost_mock.py
    test_basic.py
)

END()
