PY3_LIBRARY()

OWNER(yagafarov sparkle)

PY_SRCS(
    __init__.py
    session_builder.py
)

PEERDIR(
    alice/megamind/library/session/protos
)

END()
