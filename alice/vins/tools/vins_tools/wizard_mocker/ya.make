PY2_PROGRAM()

OWNER(g:alice)


PEERDIR(
    alice/vins/core
)

PY_SRCS(
    TOP_LEVEL
    mocker.py
)

PY_MAIN(mocker)

END()
