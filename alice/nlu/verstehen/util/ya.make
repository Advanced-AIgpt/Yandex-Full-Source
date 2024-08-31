PY2_LIBRARY()

OWNER(
    artemkorenev
    g:alice_quality
)

PEERDIR(
    # 3rd party
    bindings/python/dssm_nn_applier_lib/lib
    contrib/python/numpy
    contrib/python/requests
    contrib/python/scikit-learn
)

PY_SRCS(
    NAMESPACE verstehen.util
    __init__.py
    dssm_util.py
    generic_util.py
    granet_util.py
    multiprocessing_util.py
    numpy_util.py
)

END()
