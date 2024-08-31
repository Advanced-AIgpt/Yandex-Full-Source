LIBRARY()

OWNER(
    samoylovboris
    g:alice_quality
)

SRCS(
    rule_input.cpp
)

PEERDIR(
    contrib/libs/protobuf
    alice/begemot/lib/api/experiments
    alice/begemot/lib/api/params
    search/begemot/apphost
    search/begemot/apphost/protos
)

END()
