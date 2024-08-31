PY3TEST()

OWNER(
    g: milab
    lvlasenkov
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/integration/generator_common.inc)

PEERDIR(
    alice/hollywood/library/scenarios/transform_face/it
)

TEST_SRCS(
    generator.py
)

DATA(
    arcadia/alice/hollywood/library/scenarios/transform_face/it/data
)

END()
