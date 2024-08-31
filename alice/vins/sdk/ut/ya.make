PY2TEST()

OWNER(g:alice)

REQUIREMENTS(network:full)

INCLUDE(${ARCADIA_ROOT}/alice/vins/tests_env.inc)

PEERDIR(alice/vins/sdk)

SRCDIR(alice/vins/sdk)

TEST_SRCS(vins_sdk/tests.py)

END()
