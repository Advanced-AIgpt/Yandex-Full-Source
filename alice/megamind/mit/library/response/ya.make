PY3_LIBRARY()

OWNER(
    g:megamind
    yagafarov
)

PEERDIR(
    alice/megamind/mit/library/util
)

PY_SRCS(
    __init__.py
    wrapper.py
)

END()
