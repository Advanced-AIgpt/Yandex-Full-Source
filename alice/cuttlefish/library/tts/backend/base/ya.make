LIBRARY()

OWNER(g:voicetech-infra)

SRCS(
    callbacks_handler.cpp
    callbacks_with_eventlog.cpp
    fake.cpp
    interface.cpp
    metrics.cpp
    protobuf.cpp
    service.cpp
    service_with_eventlog.cpp
)

PEERDIR(
    alice/cuttlefish/library/apphost
    alice/cuttlefish/library/protos
    alice/cuttlefish/library/rtlog

    apphost/api/service/cpp

    contrib/libs/protobuf

    library/cpp/neh/asio
)

END()
