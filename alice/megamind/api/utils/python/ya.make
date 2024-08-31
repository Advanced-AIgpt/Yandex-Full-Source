PY3_LIBRARY()

OWNER(
    dolgawin
    g:megamind
)

PY_SRCS(
    __init__.py
    converter.pyx
)

PEERDIR(
    alice/megamind/api/utils
    alice/megamind/protos/common
)

END()

RECURSE_FOR_TESTS(
    ut
)
