UNITTEST()

OWNER(
    gusev-p
)

NEED_CHECK()

PEERDIR(
    alice/rtlog/client
    alice/rtlog/common/eventlog
)

SRCS(
    client_ut.cpp
)

END()

RECURSE(
    python
)
