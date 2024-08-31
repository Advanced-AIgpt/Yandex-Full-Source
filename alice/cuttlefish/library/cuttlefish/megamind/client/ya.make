LIBRARY()
OWNER(g:voicetech-infra)

SRCS(
    http_requester.cpp
    client.cpp
)

PEERDIR(
    alice/cuttlefish/library/cuttlefish/common
    alice/cuttlefish/library/cuttlefish/config
    alice/cuttlefish/library/cuttlefish/megamind/request
    alice/cuttlefish/library/experiments

    alice/cuttlefish/library/logging
    alice/cuttlefish/library/protos
    alice/cuttlefish/library/rtlog

    alice/library/json

    alice/megamind/api/response

    alice/megamind/protos/common
    alice/megamind/protos/speechkit

    library/cpp/json/writer
    library/cpp/neh
    library/cpp/threading/future
    library/cpp/uri
)

END()

RECURSE_FOR_TESTS(ut)
