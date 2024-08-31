LIBRARY()

OWNER(
    g:megamind
    g-kostin
)

PEERDIR(
    alice/library/experiments
    alice/library/network
    alice/library/proto
    alice/library/scenarios/utils
    alice/megamind/library/apphost_request
    alice/megamind/library/config
    alice/megamind/library/experiments
    alice/megamind/library/scenarios/interface
    alice/megamind/library/sources
    alice/megamind/library/util
    alice/megamind/protos/scenarios
    contrib/libs/protobuf
    library/cpp/langs
    library/cpp/string_utils/base64
)

SRCS(
    helpers.cpp
    protocol_scenario.cpp
    metrics.cpp
)

END()

RECURSE_FOR_TESTS(ut)
