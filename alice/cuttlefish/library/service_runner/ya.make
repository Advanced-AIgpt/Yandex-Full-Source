LIBRARY()

OWNER(voicetech)

PEERDIR(
    alice/cuttlefish/library/apphost
    alice/cuttlefish/library/logging
    alice/cuttlefish/library/metrics
    alice/cuttlefish/library/mlock
    alice/cuttlefish/library/proto_configs
    alice/cuttlefish/library/protos
    alice/cuttlefish/library/rtlog

    apphost/api/service/cpp

    voicetech/library/evlogdump

    contrib/libs/protobuf

    library/cpp/getopt/small
    library/cpp/neh
    library/cpp/proto_config
    library/cpp/svnversion
    library/cpp/unistat
)

SRCS(
    service_runner.cpp
)

END()
