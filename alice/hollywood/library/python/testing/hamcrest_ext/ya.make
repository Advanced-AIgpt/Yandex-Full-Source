PY3_LIBRARY()

OWNER(
    g:hollywood
    vitvlkv
)

PEERDIR(
    contrib/python/PyHamcrest
)

PY_SRCS(
    __init__.py
    is_dict_containing_only_entries.py
    is_non_empty_dict.py
)

END()

RECURSE_FOR_TESTS(
    tests
)
