OWNER(g:paskills)

PY3TEST()

SIZE(SMALL)

TEST_SRCS(
    conftest.py
    test.py
)

DATA(
    arcadia/alice/paskills/granet_server/config
)

DEPENDS(
    alice/paskills/granet_server/server
)

PEERDIR(
    contrib/python/requests
)

END()
