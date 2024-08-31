PY3_PROGRAM(build_hnsw)

OWNER(
    nzinov
    g:alice_boltalka
)

PY_MAIN(main)

PY_SRCS(
    TOP_LEVEL
    main.py
)

PEERDIR(
    contrib/python/PyYAML
    yt/python/client
    nirvana/valhalla/src
)

END()
