PY3_LIBRARY()

OWNER(mihajlova)

PY_SRCS(
    __init__.py
    translit.py
)

PEERDIR(
    contrib/python/transliterate
)

END()
