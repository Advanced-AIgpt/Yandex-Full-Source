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
    alice/cuttlefish/library/apphost
    alice/cuttlefish/library/protos
    alice/cuttlefish/library/rtlog

    apphost/api/service/cpp

    voicetech/asr/engine/proto_api

    contrib/libs/protobuf

    library/cpp/threading/atomic
    library/cpp/threading/future
)

END()
