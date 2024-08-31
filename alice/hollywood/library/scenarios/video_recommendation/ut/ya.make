UNITTEST_FOR(alice/hollywood/library/scenarios/video_recommendation)

SIZE(MEDIUM)

OWNER(
    dan-anastasev
    g:megamind
)

PEERDIR(
    alice/hollywood/library/resources
    alice/hollywood/library/scenarios/video_recommendation/proto
    alice/library/client/protos
    alice/library/json
    alice/library/request
    alice/library/unittest
    alice/megamind/protos/common
    alice/megamind/protos/scenarios
    alice/protos/data/video
    kernel/dssm_applier/nn_applier/lib
    library/cpp/dot_product
    library/cpp/json
    library/cpp/resource
)

DEPENDS(
    alice/hollywood/library/scenarios/video_recommendation/ut/data
)

SRCS(
    alice/hollywood/library/scenarios/video_recommendation/embedder_ut.cpp
    alice/hollywood/library/scenarios/video_recommendation/state_updater_ut.cpp
    alice/hollywood/library/scenarios/video_recommendation/video_database_ut.cpp
)

END()
