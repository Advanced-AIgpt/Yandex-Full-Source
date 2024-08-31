OWNER(
    g:alice_quality
)

PY23_LIBRARY()

PEERDIR(
    alice/nlu/libs/request_normalizer
)

PY_SRCS(
    lang.py
    request_normalizer.pyx
)

END()

RECURSE_FOR_TESTS(test)
