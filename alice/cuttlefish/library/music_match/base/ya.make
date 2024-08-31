LIBRARY()

OWNER(g:voicetech-infra)

SRCS(
    callbacks_handler.cpp
    fake.cpp
    interface.cpp
    protobuf.cpp
    service.cpp
)

PEERDIR(
    alice/cuttlefish/library/cuttlefish/common

    alice/cuttlefish/library/apphost
    alice/cuttlefish/library/logging
    alice/cuttlefish/library/protos

    contrib/libs/protobuf
    library/cpp/threading/atomic
    library/cpp/threading/future

    apphost/api/service/cpp
)

END()
