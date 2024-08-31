PY3_LIBRARY()

OWNER(
    g:voicetech-infra
)

PY_SRCS(
    __init__.py
    delivery.py
    uniproxy.py
    notificator.py
)

PEERDIR(
    alice/uniproxy/library/perf_tester
    alice/uniproxy/library/settings
)

END()

RECURSE_FOR_TESTS(ut)
