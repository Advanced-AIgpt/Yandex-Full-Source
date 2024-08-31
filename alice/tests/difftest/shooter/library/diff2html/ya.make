LIBRARY()

OWNER(sparkle)

PEERDIR(
    alice/hollywood/library/scenarios/general_conversation/proto
    alice/hollywood/protos
    alice/joker/library/log
    alice/megamind/protos/scenarios
    contrib/libs/protobuf
    contrib/libs/re2
    apphost/lib/proto_answers
)

SRCS(
    diff2html.cpp
)

END()
