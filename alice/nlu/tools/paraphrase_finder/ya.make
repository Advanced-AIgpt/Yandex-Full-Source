PY2_PROGRAM()

OWNER(dan-anastasev)

PEERDIR(
    bindings/python/dssm_nn_applier_lib/lib
    bindings/python/lemmer_lib
    contrib/python/numpy
    contrib/python/scipy
    yt/python/client
)

PY_SRCS(
    MAIN main.py
)

END()

RECURSE(
    data
)
