LIBRARY()
OWNER(g:voicetech-infra)

SRCS(
    service.cpp
)

PEERDIR(
    alice/cuttlefish/library/cuttlefish/common
    alice/cuttlefish/library/logging

    apphost/api/service/cpp
    apphost/lib/proto_answers
)

END()

RECURSE_FOR_TESTS(ut)
