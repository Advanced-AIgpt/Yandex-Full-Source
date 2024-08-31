OWNER(
    the0
    g:alice_quality
)

PY23_LIBRARY()

PEERDIR(
    alice/nlu/libs/occurrence_searcher
    alice/nlu/proto/entities
)

PY_SRCS(
    __init__.py
    occurrence_searcher.pyx
)

END()

RECURSE_FOR_TESTS(test)
