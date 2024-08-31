LIBRARY()

OWNER(
    smirnovpavel
    g:alice_quality
)

PEERDIR(
    alice/nlu/libs/interval
    kernel/inflectorlib/phrase/simple
)

SRCS(
    token_interval_inflector.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
