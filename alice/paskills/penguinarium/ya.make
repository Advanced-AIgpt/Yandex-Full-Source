PY3_LIBRARY()

OWNER(
    g:paskills
    penguin-diver
)

PY_SRCS(
    init_app.py
    routes.py

    ml/embedder.py
    ml/index.py
    ml/intent_resolver.py

    storages/graph.py
    storages/nodes.py
    storages/tables.py
    storages/ydb_utils.py

    views/base.py
    views/graph.py
    views/intent.py
    views/middleware.py
    views/misc.py
    views/nodes.py

    util/metrics.py
)

PEERDIR(
    alice/paskills/penguinarium/dssm_applier

    contrib/python/aiohttp
    contrib/python/aioredis/aioredis-1
    contrib/python/cachetools
    contrib/python/gunicorn
    contrib/python/jsonschema
    contrib/python/Jinja2
    contrib/python/numpy
    contrib/python/scikit-learn
    contrib/python/uvloop

    ydb/public/sdk/python
    kikimr/public/sdk/python/tvm
)

END()

RECURSE(
    app
    dssm_applier
    models
)

RECURSE_FOR_TESTS(
    ml/ut
    storages/it
    storages/ut
    views/it
    views/ut
    util/ut
)
