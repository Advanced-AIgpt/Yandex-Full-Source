PY23_LIBRARY()

OWNER(
    akastornov
    gusev-p
)

SRCDIR(alice/rtlog/client/python)

PY_SRCS(
    TOP_LEVEL
    rtlog/__init__.py
    rtlog/_handler.py
    rtlog/thread_local.py
    rtlog/_client.pyx=rtlog.client
)

PEERDIR(
    library/cpp/eventlog
    alice/rtlog/client
    alice/rtlog/protos
)

END()
