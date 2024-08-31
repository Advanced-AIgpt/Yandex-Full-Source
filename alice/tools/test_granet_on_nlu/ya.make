OWNER(g:karalink)

PY2_PROGRAM(prepare_data)

PY_SRCS(
    MAIN prepare_data.py
)

PEERDIR(
    yt/python/client
    contrib/python/tqdm
    contrib/python/click
)

END()
