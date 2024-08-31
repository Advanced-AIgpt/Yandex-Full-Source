PY3_LIBRARY()

OWNER(g:alice)

PEERDIR(
    alice/acceptance/modules/request_generator/lib
    alice/memento/proto
    alice/tests/library/vault
    contrib/python/protobuf
)

PY_SRCS(
    input_dialog.py
)

END()
