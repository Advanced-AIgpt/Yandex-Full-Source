PY3_PROGRAM()

OWNER(
    nzinov
    g:alice_boltalka
)

PEERDIR(
    yt/python/client
    contrib/python/PyYAML
    contrib/python/requests
    contrib/python/numpy
)

PY_MAIN(main)

PY_SRCS(
    TOP_LEVEL
    main.py
)

END()

RECURSE(
    build_hnsw
)
