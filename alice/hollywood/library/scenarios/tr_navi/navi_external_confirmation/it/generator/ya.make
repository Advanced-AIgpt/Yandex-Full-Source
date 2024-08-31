PY3TEST()

OWNER(
    flimsywhimsy
    g:alice
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/integration/generator_common.inc)
INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/integration/localhost_bass.inc)

PEERDIR(
    alice/hollywood/library/scenarios/tr_navi/navi_external_confirmation/it
)

TEST_SRCS(generator.py)

DATA(arcadia/alice/hollywood/library/scenarios/tr_navi/navi_external_confirmation/it/data)

END()
