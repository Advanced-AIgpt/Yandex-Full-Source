PY23_LIBRARY()

OWNER(alexanderplat g:alice)

PEERDIR(
    alice/nlg/library/python/nlg_renderer/bindings
)

PY_SRCS(
    __init__.py
)

END()

RECURSE(
    bindings
)

RECURSE_FOR_TESTS(
    ut
)
