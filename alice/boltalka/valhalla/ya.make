PY2_LIBRARY()

OWNER(
    krom
    g:alice_boltalka
)

PY_SRCS(
    operations.py
    utils.py
)

PEERDIR(
    yt/python/client
    nirvana/valhalla/src
    library/python/resource
)

RESOURCE(
    alice/boltalka/valhalla/ru_cfg.json ru_cfg.json
    alice/boltalka/valhalla/tr_cfg.json tr_cfg.json
    alice/boltalka/valhalla/ru_test_cfg.json ru_test_cfg.json
)

END()

RECURSE(
    assess_pool
    exact_applier
    train_reranker
    lib
    pack
)
