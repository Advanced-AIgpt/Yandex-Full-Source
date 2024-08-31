UNITTEST_FOR(alice/megamind/library/session)

OWNER(g:megamind)

PEERDIR(
    alice/library/frame
    alice/library/unittest
    alice/megamind/library/response
    alice/megamind/library/scenarios/features
    alice/megamind/library/stack_engine
    alice/megamind/library/testing
    contrib/libs/protobuf
)

SRCS(
    alice/megamind/library/session/dialog_history_ut.cpp
    alice/megamind/library/session/session_ut.cpp
)

END()
