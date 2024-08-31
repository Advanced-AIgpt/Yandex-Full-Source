PY3_LIBRARY()

OWNER(
    nzinov
    g:alice_boltalka
)

PY_SRCS(
    main.py
)

PEERDIR(
    contrib/python/requests
    contrib/python/urllib3
)

END()