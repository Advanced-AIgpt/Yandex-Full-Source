PY3TEST()
OWNER(g:voicetech-infra)

PEERDIR(
    contrib/python/tornado/tornado-4
    alice/uniproxy/library/testing
    alice/uniproxy/library/utils
)

DEPENDS(
    alice/uniproxy/bin/uniproxy
    passport/infra/tools/tvmknife/bin
)

DATA(
    arcadia/alice/uniproxy/bin/uniproxy/tests/data/settings.json
)

TEST_SRCS(
    run.py
    common.py
    test_smoke.py
    test_experiments.py
    test_uniproxy2.py
)

END()
