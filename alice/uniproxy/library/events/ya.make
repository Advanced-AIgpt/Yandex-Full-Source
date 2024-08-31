PY3_LIBRARY()

OWNER(
    g:voicetech-infra
)

PY_SRCS(
    __init__.py
    directive.py
    event_exception.py
    event.py
    go_away.py
    invalid_auth.py
    streamcontrol.py
    extra.py
)

PEERDIR(
    alice/uniproxy/library/settings
    alice/cuttlefish/library/protos
)

END()

RECURSE_FOR_TESTS(
    ut
)
