PY3_LIBRARY()

STYLE_PYTHON()

OWNER(g:voicetech-infra)

PEERDIR(
    alice/cuttlefish/library/surface_mapper
)

PY_SRCS(
    __init__.py
    bindings.pyx
    mapper.py
)

END()

RECURSE_FOR_TESTS(
    ut
)
