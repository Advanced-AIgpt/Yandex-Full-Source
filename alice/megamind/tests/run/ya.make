PY3_PROGRAM()

OWNER(g:megamind)

PEERDIR(
    alice/megamind/tests/library
    alice/library/python/utils
)

PY_SRCS(
    MAIN __main__.py
)

END()
