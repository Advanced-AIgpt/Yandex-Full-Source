LIBRARY()

OWNER(g:voicetech-infra)

SRCS(
    music_match.cpp
    music_match_callbacks_with_eventlog.cpp
    music_match_client.cpp
    service.cpp
    test_functions.cpp
    unistat.cpp
)

RESOURCE(
    default_config.json /music_match_adapter/default_config.json
)

PEERDIR(
    alice/cuttlefish/library/logging
    alice/cuttlefish/library/music_match/base
    alice/cuttlefish/library/proto_configs
    alice/cuttlefish/library/protos
    alice/cuttlefish/library/speechkit_proto

    voicetech/library/asconv
    voicetech/library/common
    voicetech/library/idl/log
    voicetech/library/proto_api
    voicetech/library/protobuf_handler
    voicetech/library/ws_server

    apphost/api/service/cpp

    contrib/libs/protobuf

    library/cpp/neh
    library/cpp/proto_config
    library/cpp/threading/atomic
)

END()

RECURSE_FOR_TESTS(
    ut
)
