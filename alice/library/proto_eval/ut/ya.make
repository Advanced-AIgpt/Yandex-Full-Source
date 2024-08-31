OWNER(
    g:alice
    lavv17
)

UNITTEST_FOR(alice/library/proto_eval)

SRCS(
    proto_eval_ut.cpp
)

PEERDIR(
    alice/library/json
    alice/library/proto_eval/proto
    alice/megamind/protos/common
    alice/megamind/protos/scenarios
)

END()
