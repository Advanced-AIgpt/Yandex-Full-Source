PY3_PROGRAM()

OWNER(
    vl-trifonov
    g:alice_quality
)

PEERDIR(
    alice/nlu/proto/dataset_info
    contrib/python/click
    yt/python/client
)

PY_SRCS(
    MAIN main.py
)

END()
