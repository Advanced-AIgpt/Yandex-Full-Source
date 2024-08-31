PY2TEST()

OWNER(g:megamind)

TEST_SRCS(
    test_lemmer.py
    test_nlu_formats.py
    test_utils.py
)


RESOURCE_FILES(
    first_name_ru.txt
)


PEERDIR(
    alice/nlu/py_libs/utils
    contrib/python/numpy
    contrib/python/pytest
    library/python/resource
)


SIZE(SMALL)

END()
