PY3_PROGRAM()

OWNER(
    samoylovboris
    g:alice_quality
)

PEERDIR(
    contrib/libs/tf/python
    contrib/python/attrs
    library/python/base64
    contrib/python/click
    library/python/json
    contrib/python/numpy
    contrib/python/requests
    yt/python/client
)

PY_SRCS(
    MAIN __main__.py
    dataset_loader.py
    dataset_prepare.py
    dataset.py
    embeddings.py
    metrics_counter.py
    model_input.py
    model.py
    stub_fetcher.py
    test_model.py
    train_model.py
    utils.py
)

END()
