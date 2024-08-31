PY3_LIBRARY()

OWNER(
    alkapov
)

PEERDIR(
    alice/beggins/cmd/manifestator/lib

    nirvana/vh3/src
    yt/python/client

    contrib/python/numpy
    contrib/python/scipy
    contrib/python/scikit-learn

    ml/pulsar/python-package
)

PY_SRCS(
    ext.py
    public.py
    logs_classifier.py
)

END()
