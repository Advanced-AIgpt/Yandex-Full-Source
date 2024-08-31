LIBRARY()

OWNER(
    yorky0
    g:alice_quality
)

PEERDIR(
    alice/nlu/granet/lib/sample
    alice/nlu/granet/lib/user_entity
    library/cpp/langs
    search/begemot/rules/alice/session/proto
)

SRCS(
    utils.cpp
)

END()
