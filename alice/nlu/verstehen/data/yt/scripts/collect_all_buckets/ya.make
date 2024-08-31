PY2_PROGRAM(collect_all_buckets)

OWNER(
    artemkorenev
    g:alice_quality
)

PEERDIR(
    # 3rd party
    nirvana/valhalla/src
)

PY_SRCS(
    NAMESPACE verstehen.data.yt.scripts.collect_all_buckets
    __main__.py
)

END()
