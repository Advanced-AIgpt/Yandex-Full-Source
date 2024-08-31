LIBRARY()
OWNER(g:voicetech-infra)

SRCS(
    auth.cpp
    flags_json.cpp
    service.cpp
    tvm.cpp
    utils.cpp
)

PEERDIR(
    alice/cuttlefish/library/cuttlefish/common
    alice/cuttlefish/library/cuttlefish/config
    alice/cuttlefish/library/cuttlefish/converter
    alice/cuttlefish/library/cuttlefish/stream_converter

    alice/cuttlefish/library/surface_mapper
    alice/cuttlefish/library/logging
    alice/cuttlefish/library/protos

    alice/megamind/protos/common

    alice/uniproxy/library/uaas_mapper

    voicetech/library/uniproxy2/dns

    laas/lib/ip_properties/proto

    library/cpp/string_utils/quote

    apphost/api/service/cpp
    apphost/lib/proto_answers

    yweb/webdaemons/icookiedaemon/icookie_lib
    yweb/webdaemons/icookiedaemon/icookie_lib/utils
)

END()

RECURSE_FOR_TESTS(ut)
