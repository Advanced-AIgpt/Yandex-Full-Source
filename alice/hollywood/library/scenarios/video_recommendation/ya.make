LIBRARY()

OWNER(
    dan-anastasev
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/base_scenario
    alice/hollywood/library/global_context
    alice/hollywood/library/registry
    alice/hollywood/library/request
    alice/hollywood/library/resources
    alice/hollywood/library/response
    alice/hollywood/library/scenarios/video_recommendation/nlg
    alice/hollywood/library/scenarios/video_recommendation/proto
    alice/library/client
    alice/library/client/protos
    alice/library/json
    alice/library/logger
    alice/library/video_common
    alice/megamind/protos/common
    alice/megamind/protos/scenarios
    alice/protos/data/video
    kernel/dssm_applier/nn_applier/lib
    library/cpp/dot_product
    library/cpp/json
    library/cpp/resource
    library/cpp/timezone_conversion
)

SRCS(
    embedder.cpp
    GLOBAL handle.cpp
    state_updater.cpp
    video_database.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
