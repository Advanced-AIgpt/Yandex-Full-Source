GO_TEST_FOR(alice/iot/time_machine/server)

OWNER(g:alice_iot)

SIZE(MEDIUM)

REQUIREMENTS(network:full)

ENV(YA_MAKE_TEST_RUN=1)

DATA(arcadia/alice/library/go/queue/pgbroker/db)

TEST_CWD(alice/library/go/queue/pgbroker/)

END()
