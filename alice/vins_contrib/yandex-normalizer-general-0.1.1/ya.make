PY2_LIBRARY()

OWNER(
    akastornov
    g:alice
)

PEERDIR(
    contrib/python/pyfst
)

PY_SRCS(
    TOP_LEVEL
    normalizer_general/ru/__init__.py
    normalizer_general/ru/normalization/__init__.py
    normalizer_general/general/__init__.py
    normalizer_general/general/normbase.py
    normalizer_general/fr/__init__.py
    normalizer_general/__init__.py
    normalizer_general/tr/__init__.py
    normalizer_general/tr/normalization/__init__.py
    normalizer_general/en/__init__.py
    normalizer_general/en/normalization/__init__.py
    normalizer_general/uk/__init__.py
    normalizer_general/uk/normalization/__init__.py
)

END()
