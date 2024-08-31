LIBRARY()
OWNER(g:voicetech-infra)

SRCS(
    service.cpp
    util.cpp
)

PEERDIR(
    alice/cuttlefish/library/cuttlefish/common

    alice/cuttlefish/library/apphost
    alice/cuttlefish/library/logging

    voicetech/library/common

    apphost/api/service/cpp

    contrib/libs/re2

    library/cpp/xml/document
)

END()

RECURSE_FOR_TESTS(ut)
