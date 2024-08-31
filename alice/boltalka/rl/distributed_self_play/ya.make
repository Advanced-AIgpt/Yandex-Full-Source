PY3_PROGRAM()

PY_SRCS(
    lib.py
    TOP_LEVEL
    run.py
)

PY_MAIN(run)

PEERDIR(
    nirvana/valhalla/src
)

END()
