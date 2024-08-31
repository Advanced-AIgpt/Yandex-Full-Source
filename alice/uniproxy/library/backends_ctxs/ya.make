PY3_LIBRARY()

OWNER(
    g:voicetech-infra
)

PY_SRCS(
    contacts.py
    contexts.py
    smart_home.py
    memento.py
)

PEERDIR(
    alice/memento/proto
    alice/uniproxy/library/auth
    alice/uniproxy/library/logging
    contrib/python/tornado/tornado-4
)

END()

RECURSE_FOR_TESTS(ut)
