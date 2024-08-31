UNITTEST_FOR(alice/megamind/library/scenarios/helpers)

OWNER(g:megamind)

PEERDIR(
    alice/library/frame
    alice/library/json
    alice/library/response_similarity
    alice/library/unittest
    alice/megamind/library/context
    alice/megamind/library/experiments
    alice/megamind/library/factor_storage
    alice/megamind/library/memento
    alice/megamind/library/scenarios/protocol
    alice/megamind/library/testing
    alice/megamind/protos/scenarios
    alice/megamind/protos/speechkit
    contrib/libs/protobuf
    kernel/geodb
    library/cpp/cgiparam
    library/cpp/testing/gmock_in_unittest
    library/cpp/json
)

SRCS(
    scenario_api_helper_ut.cpp
    scenario_wrapper_ut.cpp
)

END()
