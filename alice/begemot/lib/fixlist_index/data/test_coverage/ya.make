OWNER(
    glebovch
    g:alice_quality
)

EXECTEST()

DEPENDS(
    alice/begemot/lib/fixlist_index/data/test_coverage/pool
    alice/begemot/lib/fixlist_index/data/test_coverage/test_runner
)

DATA(
    arcadia/alice/begemot/lib/fixlist_index/data/ru
    arcadia/alice/begemot/lib/fixlist_index/data/test_coverage/pool
)

RUN(
    NAME test_coverage
    test_runner --data alice5v3.tsv
)

SIZE(MEDIUM)

END()

RECURSE(
    test_runner
    pool
)
