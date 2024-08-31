LIBRARY()

OWNER(g:voicetech-infra)

SRCS(
    rtlog.cpp
)

PEERDIR(
    alice/cuttlefish/library/proto_configs
    alice/rtlog/client

    apphost/api/service/cpp

    library/cpp/neh
)

END()
