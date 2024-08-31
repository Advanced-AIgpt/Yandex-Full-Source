PY2_PROGRAM(verstehen_local_data_preparation)

OWNER(
    artemkorenev
    g:alice_quality
)

PEERDIR(
    alice/nlu/verstehen/config
    alice/nlu/verstehen/preprocess
    alice/nlu/verstehen/util
)

PY_SRCS(
    NAMESPACE verstehen.data.local
    __main__.py
    __init__.py
    prepare_metrics_data.py
    prepare_prod_data.py
    util.py
)

END()
