UNITTEST_FOR(alice/nlu/libs/entity_searcher)

OWNER(
    yorky0
    g:alice_quality
)

PEERDIR(
    library/cpp/containers/comptrie
    library/cpp/packers
)

SRCS(
    entity_searcher_builder_ut.cpp
    sample_graph_ut.cpp
    entity_searcher_ut.cpp
)

END()
