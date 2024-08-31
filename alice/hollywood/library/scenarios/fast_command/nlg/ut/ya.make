UNITTEST_FOR(alice/hollywood/library/scenarios/fast_command/nlg)

OWNER(nkodosov)

SRCS(
    test_all.cpp
)

DATA(
    arcadia/alice/hollywood/shards/all/prod/common_resources/nlg_translations.json
)

PEERDIR(
    alice/nlg/library/testing
    library/cpp/testing/unittest
)

END()
