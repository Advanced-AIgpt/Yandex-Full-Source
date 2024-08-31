PY2_PROGRAM(gen_ammo)

OWNER(g:alice)

PEERDIR(
    alice/vins/core
    alice/vins/apps/navi
    yt/python/client
    contrib/python/click
)

PY_SRCS(
    gen_ammo.py=__main__
)

END()
