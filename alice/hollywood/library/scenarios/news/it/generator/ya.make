PY3TEST()

OWNER(
    g:hollywood
    khr2
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/integration/generator_common.inc)
INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/integration/localhost_bass.inc)

PEERDIR(
    alice/hollywood/library/scenarios/news/it
)

TEST_SRCS(
    generator_push.py
    generator.py
)

DATA(
    arcadia/alice/hollywood/library/scenarios/news/it/data_push
    arcadia/alice/hollywood/library/scenarios/news/it/data
)

END()
