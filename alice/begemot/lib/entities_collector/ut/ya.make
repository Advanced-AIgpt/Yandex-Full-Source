UNITTEST_FOR(alice/begemot/lib/entities_collector)

OWNER(
    samoylovboris
    g:alice_quality
)

SRCS(
    date_ut.cpp
    entity_finder_ut.cpp
)

PEERDIR(
    alice/library/proto
    library/cpp/scheme
)

END()
