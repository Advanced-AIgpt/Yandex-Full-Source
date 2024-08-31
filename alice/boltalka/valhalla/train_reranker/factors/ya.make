PY23_LIBRARY()

OWNER(
    krom
    g:alice_boltalka
)

PY_SRCS(
    main.py
)

PEERDIR(
    yt/python/client
    nirvana/valhalla/src
    contrib/python/numpy
)

END()
