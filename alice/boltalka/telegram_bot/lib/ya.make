PY3_LIBRARY()

OWNER(
    nzinov
    g:alice_boltalka
)

PY_SRCS(
    __init__.py
    module.py
    rpc.py
    rpc_source.py
    cache.py
    interests_ranker.py
    lstm_memory.py
    nlgsearch_http_source.py
    registered_modules.py
    replier.py
    instance.py
)

PEERDIR(
    contrib/python/requests
    contrib/python/numpy
    contrib/python/pandas
    contrib/python/PyYAML
    alice/boltalka/memory/lstm_dssm/py_apply
    alice/boltalka/py_libs/apply_nlg_dssm
    alice/boltalka/extsearch/query_basesearch/lib
)

NO_CHECK_IMPORTS()

END()
