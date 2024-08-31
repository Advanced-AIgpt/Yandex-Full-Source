PY3_PROGRAM()

OWNER(
    alkapov
)

PY_SRCS(__main__.py)

PEERDIR(
    alice/megamind/protos/scenarios
    contrib/python/attrs
    contrib/python/click
    contrib/python/tqdm
)

END()
