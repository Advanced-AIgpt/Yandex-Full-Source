OWNER(
    dan-anastasev
    g:hollywood
)

PY2_PROGRAM(
    load_movie_infos
)

PY_SRCS(
    __init__.py
    common.py
    MAIN load_movie_infos.py
)

PEERDIR(
    contrib/python/attrs
    contrib/python/numpy
    contrib/python/requests
    contrib/python/tqdm
    yt/python/client
    ydb/public/sdk/python
)

END()
