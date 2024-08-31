PY3_LIBRARY()

OWNER(
    ran1s
    g:alice
)

PEERDIR(
    search/tunneller/libs/protocol_decl
)

PY_SRCS(
    checkers.py
    utils.py
)

END()
