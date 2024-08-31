PY3_PROGRAM()

OWNER(
    nzinov
    g:alice_boltalka
)

PY_SRCS(TOP_LEVEL generate_negatives.py)

PY_MAIN(generate_negatives)

PEERDIR(
    yt/python/client
    alice/boltalka/tools/dssm_preprocessing/preprocessing/lib
)

END()
