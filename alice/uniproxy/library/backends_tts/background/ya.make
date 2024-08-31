PY3_LIBRARY()
OWNER(g:voicetech-infra)

PY_SRCS(
    __init__.py
)

PEERDIR(
    library/python/resource
)

FROM_SANDBOX(
    2231787408
    OUT_NOAUTO
        data/birds.pcm
        data/birds2.pcm
        data/rain.pcm
        data/rain2.pcm
        data/meadow.pcm
        data/meadow2.pcm
)


RESOURCE(
    data/birds.pcm      /backgrounds/birds.pcm
    data/birds2.pcm     /backgrounds/birds2.pcm
    data/rain.pcm       /backgrounds/rain.pcm
    data/rain2.pcm      /backgrounds/rain2.pcm
    data/meadow.pcm     /backgrounds/meadow.pcm
    data/meadow2.pcm    /backgrounds/meadow2.pcm
)

END()
