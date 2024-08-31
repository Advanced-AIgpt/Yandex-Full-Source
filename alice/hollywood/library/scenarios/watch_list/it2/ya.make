PY3TEST()

OWNER(
    g:smarttv
)

SIZE(MEDIUM)

FORK_SUBTESTS()
SPLIT_FACTOR(1)
SET(HOLLYWOOD_SHARD video)
ENV(HOLLYWOOD_SHARD=video)

PEERDIR(
    alice/protos/data/tv/watch_list
)

TEST_SRCS(
    tests.py
)

DATA(
    arcadia/alice/hollywood/library/scenarios/watch_list/it2/tests
)

REQUIREMENTS(ram:10)

END()
