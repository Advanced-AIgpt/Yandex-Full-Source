OWNER(
    dan-anastasev
    g:hollywood
    g:alice_boltalka
)

PY3_PROGRAM(
    prepare_known_movies
)

PY_SRCS(
    MAIN main.py
)

PEERDIR(
    contrib/python/PyYAML
    contrib/python/tqdm
    yt/python/client
)

END()
