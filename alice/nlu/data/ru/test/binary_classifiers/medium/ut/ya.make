PY3TEST()

DATA(
    arcadia/alice/nlu/data/ru/models/alice_binary_intent_classifier/data.inc
    arcadia/alice/nlu/data/ru/test/binary_classifiers/medium/target
)

TAG(ya:external ya:fat ya:force_sandbox ya:not_autocheck)

REQUIREMENTS(
    network:full
)

SIZE(LARGE)

PEERDIR(
    alice/nlu/data/ru/test/binary_classifiers/common

    contrib/python/pytest

    yt/python/client
    yql/library/python
)

TEST_SRCS(
    test.py
)

END()
