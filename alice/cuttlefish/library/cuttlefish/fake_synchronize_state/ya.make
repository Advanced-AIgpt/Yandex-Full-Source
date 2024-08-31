LIBRARY()
OWNER(g:voicetech-infra)

SRCS(
    service.h
    service.cpp
)

PEERDIR(
    alice/cuttlefish/library/cuttlefish/common
    alice/cuttlefish/library/cuttlefish/synchronize_state
    alice/cuttlefish/library/logging

    # alice/memento/proto

    apphost/api/service/cpp
    # apphost/lib/proto_answers
)

END()
