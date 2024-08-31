PY3_LIBRARY()

OWNER(
    g:matrix
)

PEERDIR(
    alice/matrix/worker/library/services/worker/protos

    alice/matrix/library/testing/python
)

PY_SRCS(
    constants.py
    matrix_notificator_mock.py
    matrix_worker.py
    proto_builder_helpers.py
    test_base.py
    ydb.py
)

END()
