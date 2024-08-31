PY3_PROGRAM(prepare)

OWNER(
    lvlasenkov
    g:milab
)

PY_SRCS(
    __main__.py
)

PEERDIR(
    contrib/python/pymongo
    library/python/vault_client
    alice/hollywood/library/scenarios/draw_picture/resources/proto
)

END()
