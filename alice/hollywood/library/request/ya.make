LIBRARY()

OWNER(
    akhruslan
    g:hollywood
)

SRCS(
    request.cpp
    experiments.cpp
)

PEERDIR(
    alice/hollywood/library/frame
    alice/hollywood/library/framework/proto
    alice/hollywood/library/util
    alice/library/client
    alice/library/geo_resolver
    alice/library/restriction_level
    alice/library/scenarios/data_sources
    alice/library/util
    alice/megamind/protos/scenarios
    apphost/api/service/cpp
    contrib/libs/protobuf
    library/cpp/langs
)

END()

RECURSE_FOR_TESTS(
    ut
)

RECURSE(
    utils
)
