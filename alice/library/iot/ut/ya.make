UNITTEST_FOR(alice/library/iot)

OWNER(
    micyril
    g:alice
)

SIZE(MEDIUM)

PEERDIR(
    library/cpp/scheme
    alice/library/iot
    alice/bass/libs/logging_v2
    alice/library/json
    alice/megamind/protos/common
    library/cpp/resource
    alice/nlu/libs/iot
    library/cpp/iterator
)

SRCS(
    ut.cpp
)

RESOURCE(
    tests.json tests.json
)

END()
