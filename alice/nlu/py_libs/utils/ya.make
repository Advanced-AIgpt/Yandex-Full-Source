PY2_LIBRARY()

OWNER(g:alice)

PEERDIR(
    bindings/python/inflector_lib
    bindings/python/lemmer_lib
    contrib/python/attrs
    contrib/python/numpy
    contrib/python/transliterate
)

PY_SRCS(
    __init__.py
    fuzzy_nlu_format.py
    lemmer.py
    sample_normalizer.py
    sample.py
    sampling.py
    slot.py
    strings.py
)

END()

RECURSE_FOR_TESTS(ut)
