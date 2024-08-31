PY3_LIBRARY()

OWNER(
    g:matrix
)

PEERDIR(
    alice/matrix/notificator/library/storages/connections/protos

    alice/matrix/library/testing/protos
    alice/matrix/library/testing/python

    alice/megamind/protos/common
    alice/megamind/protos/scenarios
    alice/protos/api/notificator
    alice/protos/data/device
    alice/uniproxy/library/protos

    contrib/python/pytest-asyncio
)

PY_SRCS(
    constants.py
    iot_mock.py
    matrix.py
    proto_builder_helpers.py
    python_notificator.py
    subway_mock.py
    test_base.py
    ydb.py

    utils.pyx
)

END()
