PY23_LIBRARY()

OWNER(g:wonderlogs)

PY_SRCS(
    getters.pyx
)

PEERDIR(
    alice/wonderlogs/sdk/core
    alice/megamind/protos/analytics
    alice/megamind/protos/speechkit
    alice/library/json
)

END()

RECURSE_FOR_TESTS(ut)
