PY2TEST()

OWNER(
    alipov
    g:alice_boltalka
)

TEST_SRCS(test.py)

SIZE(LARGE)
TAG(ya:fat ya:force_sandbox)

DEPENDS(
    alice/boltalka/extsearch/base/nlgsearch
)

DATA(
    sbr://981363863 # index
    arcadia/alice/boltalka/extsearch/tests/bucket.tskv
    arcadia/alice/boltalka/extsearch/base/nlgsearch/search.cfg
    arcadia/alice/boltalka/extsearch/scripts/query_basesearch.py
)

END()
