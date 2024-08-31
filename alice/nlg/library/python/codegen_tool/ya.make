PY2_LIBRARY()

OWNER(g:alice)

PEERDIR(
    alice/nlg/library/python/codegen
    alice/vins/core/vins_core/nlg
    alice/vins/core/vins_core/utils
)

PY_SRCS(
    __init__.py
)

END()

RECURSE_FOR_TESTS(
    ut
)
