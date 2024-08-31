UNITTEST_FOR(alice/hollywood/library/scenarios/food/menu_matcher)

OWNER(
    samoylovboris
    the0
    g:alice_quality
)

PEERDIR(
    alice/hollywood/library/scenarios/food/proto
    alice/nlu/libs/tuple_like_type
    library/cpp/containers/comptrie
    library/cpp/json/writer
)

SRCS(
    matcher_ut.cpp
    string_matcher_ut.cpp
)

END()
