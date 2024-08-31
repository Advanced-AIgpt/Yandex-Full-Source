PY3_LIBRARY()

OWNER(
    g:paskills
    penguin-diver
)

PEERDIR(
    ml/dssm/lib
    ml/dssm/dssm/lib
)

PY_SRCS(
    __init__.py
    dssm_applier.pyx
)

END()

RECURSE_FOR_TESTS(
    ut
)
