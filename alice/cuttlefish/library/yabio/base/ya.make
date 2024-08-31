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
    alice/cachalot/api/protos
    alice/cuttlefish/library/apphost
    alice/cuttlefish/library/cuttlefish/common
    alice/cuttlefish/library/logging
    alice/cuttlefish/library/protos
    alice/cuttlefish/library/rtlog

    apphost/api/service/cpp

    contrib/libs/protobuf

    library/cpp/threading/atomic
)

END()
