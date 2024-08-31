LIBRARY()

OWNER(
    samoylovboris
    the0
    g:alice_quality
)

PEERDIR(
    alice/hollywood/library/scenarios/food/proto
    alice/nlu/granet/lib/utils
    alice/nlu/libs/tuple_like_type
    library/cpp/containers/comptrie
    library/cpp/json
    library/cpp/json/writer
    library/cpp/resource
)

SRCS(
    matcher.cpp
    string_matcher.cpp
)

FROM_SANDBOX(1761740178 OUT_NOAUTO menu_sample_mcdonalds_komsomolskyprospect_evening.json)

RESOURCE(
    config.json alice/hollywood/library/scenarios/food/menu_matcher/config.json
)

RESOURCE(
    menu_sample_mcdonalds_komsomolskyprospect_evening.json alice/hollywood/library/scenarios/food/menu_matcher/menu_sample_mcdonalds_komsomolskyprospect_evening.json
)

END()

RECURSE_FOR_TESTS(
    ut
)
