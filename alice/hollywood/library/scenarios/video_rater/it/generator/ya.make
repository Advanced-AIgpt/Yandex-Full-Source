PY3TEST()

OWNER(
    flimsywhimsy
    g:alice
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/integration/generator_common.inc)

PEERDIR(
    alice/hollywood/library/scenarios/video_rater/it
)

TEST_SRCS(generator.py)

DATA(arcadia/alice/hollywood/library/scenarios/video_rater/it/data)

END()
