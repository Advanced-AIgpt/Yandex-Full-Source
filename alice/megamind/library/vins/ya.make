LIBRARY()

OWNER(g:megamind)

PEERDIR(
    alice/nlu/libs/request_normalizer
)

SRCS(
    wizard.cpp
)

END()

RECURSE_FOR_TESTS(ut)
