PY3TEST()

OWNER(
    sparkle
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/megamind/mit/library/common.inc)

PEERDIR(
    alice/hollywood/library/framework/proto
    alice/hollywood/library/scenarios/music/proto
)

TEST_SRCS(
    hollywood_music.py
)

END()
