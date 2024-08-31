PY2TEST()

OWNER(
    krom
    g:alice_boltalka
)

TEST_SRCS(
    test.py
)

PEERDIR(
    alice/boltalka/extsearch/nlgsearch_simple/pytest/lib
)

DEPENDS(
    alice/boltalka/extsearch/nlgsearch_simple
)

FORK_TESTS()

DATA(
    # small_index
    sbr://1035488153

    # input.tsv
    sbr://679545343
)

REQUIREMENTS(ram:10)

END()

RECURSE(
    lib
)
