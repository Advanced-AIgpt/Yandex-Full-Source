PY2_PROGRAM(verstehen_metrics_script)

OWNER(
    artemkorenev
    g:alice_quality
)

PEERDIR(
    alice/nlu/verstehen/config
    alice/nlu/verstehen/index
    alice/nlu/verstehen/preprocess
    alice/nlu/verstehen/util
    # 3rd party
    contrib/python/numpy
    contrib/python/matplotlib
)

PY_SRCS(
    NAMESPACE verstehen.metrics
    __main__.py
    __init__.py
    measure_metrics.py
    metrics.py
    plot_writer.py
    util.py
)

END()
