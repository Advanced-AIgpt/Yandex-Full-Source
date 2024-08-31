UNITTEST_FOR(alice/megamind/library/context)

OWNER(
    g:megamind
)

PEERDIR(
    alice/library/frame
    alice/library/proto
    alice/library/unittest
    alice/megamind/library/experiments
    alice/megamind/library/testing
    alice/megamind/library/util
    library/cpp/json
)

SRCS(
    context_ut.cpp
    fixlist_ut.cpp
    parsed_frames_ut.cpp
    wizard_response_ut.cpp
)

END()
