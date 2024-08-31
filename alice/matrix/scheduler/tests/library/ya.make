PY3_LIBRARY()

OWNER(
    g:matrix
)

PEERDIR(
    alice/matrix/notificator/tests/library
    alice/matrix/scheduler/library/services/scheduler/protos

    alice/matrix/library/testing/python

    library/python/cityhash
)

PY_SRCS(
    constants.py
    matrix_scheduler.py
    proto_builder_helpers.py
    test_base.py
    ydb.py
)

END()
