UNITTEST()

OWNER(
    deemonasd
    g:alice
)

FORK_SUBTESTS()

SIZE(MEDIUM)

DATA(arcadia/alice/begemot/lib/fixlist_index/data)

SRCS(
    fixlist_index_ut.cpp
)

PEERDIR(
    alice/begemot/lib/fixlist_index
    alice/nlu/libs/request_normalizer

    library/cpp/langs
    library/cpp/testing/unittest
)

END()
