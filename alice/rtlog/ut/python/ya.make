OWNER(gusev-p)

NEED_CHECK()

PY2TEST()

TEST_SRCS(client-ut.py)

PY_SRCS(
    eventlog_wrap.pyx
)

TIMEOUT(15)

PEERDIR(
    alice/rtlog/client/python/lib
    alice/rtlog/protos
    library/cpp/eventlog/proto
)

REQUIREMENTS(ram:9)

END()
