PY3TEST()

STYLE_PYTHON()

OWNER(g:voicetech-infra)

PEERDIR(
    alice/cuttlefish/library/python/apphost_here
)

DEPENDS(
    apphost/daemons/horizon/agent
)

DATA(arcadia/apphost/conf)

TEST_SRCS(
    __init__.py
    test_validity.py
)

SIZE(MEDIUM)

TIMEOUT(180)

END()
