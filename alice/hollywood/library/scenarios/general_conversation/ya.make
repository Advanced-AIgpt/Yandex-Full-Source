LIBRARY()

OWNER(
    deemonasd
    g:hollywood
    g:alice_boltalka
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/scenarios/general_conversation/common
    alice/hollywood/library/scenarios/general_conversation/containers
    alice/hollywood/library/scenarios/general_conversation/handles
    alice/hollywood/library/scenarios/general_conversation/nlg
)

SRCS(
    GLOBAL general_conversation.cpp
)

END()

RECURSE(
    fast_data/util
    resources/prepare_known_movies
)

RECURSE_FOR_TESTS(
    it2
    long_session/ut
)
