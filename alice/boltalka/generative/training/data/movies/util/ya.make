PY3_LIBRARY()

OWNER(
    artemkorenev
    g:alice_boltalka
)

PEERDIR(
    contrib/python/tqdm
    yt/python/client
)

PY_SRCS(
    __init__.py
    util.py
)

END()
