OWNER(g:alice_analytics)

PY23_LIBRARY()

PEERDIR(
    ml/pulsar/python-package-legacy
    contrib/python/requests
)

PY_SRCS(
    pulsar_columns.py
    pulsar_urls.py
)

NO_CHECK_IMPORTS()

END()
