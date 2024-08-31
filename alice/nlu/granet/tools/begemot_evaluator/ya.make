PY2_PROGRAM(begemot_evaluator)

OWNER(
    dan-anastasev
    g:alice_quality
)

PEERDIR(
    alice/vins/core
    contrib/python/attrs
    contrib/python/tqdm
    contrib/python/scikit-learn
    yt/python/client
)

PY_SRCS(
    __main__.py
    collect_granet_responses.py
    calculate_metrics.py
)

END()
