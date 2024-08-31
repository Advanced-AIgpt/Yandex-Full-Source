PY2_PROGRAM(classifier_granet)

OWNER(kudrinsky)

PEERDIR(
    contrib/libs/tf/python
    contrib/python/numpy
    contrib/python/h5py
    contrib/python/click
    contrib/python/attrs
    contrib/python/tqdm
    contrib/python/pandas
    contrib/python/scikit-learn
    contrib/python/matplotlib
    yt/python/client
)

PY_SRCS(
    MAIN main.py
    binary_classifier.py
    dataset.py
    metrics_counter.py
    mode_handler.py
    mode_configs.py
    tf_pipelines.py
)

END()

RECURSE(
    internal_data
    #../../data/ru/intent_train/
)
