PY3TEST()

OWNER(
    ardulat
    g:alice
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/integration/generator_common.inc)
INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/integration/localhost_bass.inc)

PEERDIR(
    alice/hollywood/library/scenarios/tr_navi/get_my_location/it
)

TEST_SRCS(generator.py)

DATA(arcadia/alice/hollywood/library/scenarios/tr_navi/get_my_location/it/data)

END()
