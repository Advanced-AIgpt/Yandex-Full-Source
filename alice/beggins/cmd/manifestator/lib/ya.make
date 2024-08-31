PY3_LIBRARY()

PY_SRCS(
    process.py
)

PEERDIR(
    alice/beggins/cmd/manifestator/internal

    contrib/python/attrs
    contrib/python/PyYAML

    yt/python/client

    ml/pulsar/python-package
)

END()
