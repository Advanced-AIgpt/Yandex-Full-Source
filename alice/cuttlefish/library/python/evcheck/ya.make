PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:voicetech-infra)

PEERDIR(
    alice/cuttlefish/library/evcheck
)

PY_SRCS(
    __init__.py
    evcheck.pyx
)

END()

RECURSE_FOR_TESTS(
    ut
)
