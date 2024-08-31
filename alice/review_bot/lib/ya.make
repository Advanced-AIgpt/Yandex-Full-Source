PY23_LIBRARY()

OWNER(zubchick)

PEERDIR(
    alice/review_bot/ybot/core
    alice/review_bot/ybot/modules

    contrib/python/attrs
    contrib/python/dateutil
    contrib/python/gevent
    contrib/python/pymongo
    contrib/python/requests
    contrib/python/six

    library/python/startrek_python_client
)

PY_SRCS(
    __init__.py
    review.py
    helpers.py
    mongo.py
    startrek.py
)

END()

RECURSE_FOR_TESTS(tests)
