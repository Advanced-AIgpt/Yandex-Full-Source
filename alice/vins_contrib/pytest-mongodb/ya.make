PY23_LIBRARY()

OWNER(
    akastornov
    g:alice
)

VERSION(2.1.2)

LICENSE(MIT)

PEERDIR(
    contrib/python/PyYAML
    contrib/python/mongomock
    contrib/python/pymongo
    contrib/python/pytest
)

PY_SRCS(
    TOP_LEVEL
    pytest_mongodb/__init__.py
    pytest_mongodb/conftest.py
    pytest_mongodb/plugin.py
)

END()
