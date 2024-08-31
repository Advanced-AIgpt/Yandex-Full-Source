LIBRARY()

OWNER(g:voicetech-infra)

SRCS(
    request_builder.cpp
    hinter.cpp
)

PEERDIR(
    alice/cuttlefish/library/cuttlefish/megamind/mappers
    alice/cuttlefish/library/cuttlefish/megamind/speaker
    alice/cuttlefish/library/cuttlefish/common
    alice/cuttlefish/library/cuttlefish/config
    alice/cuttlefish/library/experiments

    alice/cachalot/api/protos
    alice/cuttlefish/library/logging
    alice/cuttlefish/library/protos
    alice/cuttlefish/library/rtlog

    alice/library/blackbox
    alice/library/experiments
    alice/library/json
    alice/protos/api/meta

    alice/megamind/protos/common
    alice/megamind/protos/speechkit

    library/cpp/json/writer
    library/cpp/neh
    library/cpp/threading/future
    library/cpp/string_utils/base64
    library/cpp/uri
)

GENERATE_ENUM_SERIALIZATION(common.h)

END()

RECURSE_FOR_TESTS(ut)
