PY3TEST()

OWNER(
    igor-darov
    g:alice_quality
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it2/common.inc)

DATA(arcadia/alice/hollywood/library/scenarios/how_to_spell/it2/tests)

TEST_SRCS(tests.py)

END()
