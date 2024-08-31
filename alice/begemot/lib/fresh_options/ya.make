LIBRARY()

OWNER(
    samoylovboris
    g:alice_quality
    g:begemot
)

PEERDIR(
    alice/begemot/lib/api/experiments
    alice/begemot/lib/fresh_options/proto
    alice/begemot/lib/rule_utils
    search/begemot/apphost
)

SRCS(
    fresh_options.cpp
)

END()
