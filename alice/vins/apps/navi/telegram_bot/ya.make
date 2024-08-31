PY2_PROGRAM()

OWNER(g:alice)

PEERDIR(
    alice/vins/apps/navi
)

PY_SRCS(
    alice/vins/apps/navi/navi_app/app.py=__main__
)

END()
