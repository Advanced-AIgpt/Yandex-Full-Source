PY3_PROGRAM(send_alice_digest)

OWNER(
    g:matrix
)

PEERDIR(
    alice/uniproxy/library/protos

    contrib/python/requests
)

PY_SRCS(
    MAIN __main__.py
)

END()
