PY23_LIBRARY()

OWNER(
    krom
    g:alice_boltalka
)

PY_SRCS(
    main.py
    TOP_LEVEL   alice/boltalka/tools/calc_reranker_metrics/utils.py
)

PEERDIR(
    yt/python/client
    nirvana/valhalla/src
    contrib/python/numpy
)

END()
