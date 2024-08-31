PY3TEST()

OWNER(sparkle)

SIZE(MEDIUM)

PEERDIR(
    alice/megamind/protos/common
    alice/nlu/py_libs/granet
)

DATA(
    arcadia/alice/nlu/data/ru/granet
)

TEST_SRCS(
    test_granet_frames.py
)

END()
