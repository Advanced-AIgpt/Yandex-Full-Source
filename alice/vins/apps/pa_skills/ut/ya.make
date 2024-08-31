PY2TEST()

OWNER(g:alice)

REQUIREMENTS(network:full)

INCLUDE(${ARCADIA_ROOT}/alice/vins/tests_env.inc)

PEERDIR(alice/vins/apps/pa_skills)

DEPENDS(alice/vins/resources)

SRCDIR(alice/vins/apps/pa_skills)

TEST_SRCS(gc_skill/tests.py)

END()
