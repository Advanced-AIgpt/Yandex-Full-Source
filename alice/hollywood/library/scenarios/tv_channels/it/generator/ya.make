PY3TEST()

SIZE(MEDIUM)

OWNER(
    igoshkin
    g:smarttv
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/integration/generator_common.inc)

PEERDIR(
    alice/hollywood/library/scenarios/tv_channels/it
)

TEST_SRCS(
    generator.py
)

DATA(
    arcadia/alice/hollywood/library/scenarios/tv_channels/it/data
)

END()
