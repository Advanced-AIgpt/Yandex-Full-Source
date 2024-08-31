GO_TEST_FOR(alice/iot/bulbasaur/server)

OWNER(g:alice_iot)

ENV(YDB_USE_IN_MEMORY_PDISKS=true)

ENV(YDB_TYPE=LOCAL)

INCLUDE(${ARCADIA_ROOT}/kikimr/public/tools/ydb_recipe/recipe_stable.inc)

DATA(arcadia/alice/iot/bulbasaur/server/testdata/cancel-all-scenarios.protobuf)

DATA(arcadia/alice/iot/bulbasaur/server/testdata/several-delayed-hypothesis-with-same-action.protobuf)

DATA(arcadia/alice/iot/bulbasaur/server/testdata/turn-off-lamp-in-1-year.protobuf)

DATA(arcadia/alice/iot/bulbasaur/server/testdata/turn-off-lamp-in-5-minutes.protobuf)

DATA(arcadia/alice/iot/bulbasaur/server/testdata/turn-off-lamp-in-23_59.protobuf)

DATA(arcadia/alice/iot/bulbasaur/server/testdata/turn-on-lamp-tomorrow-time-specify-callback.protobuf)

DATA(arcadia/alice/iot/bulbasaur/server/testdata/turn-on-lamp-tomorrow.protobuf)

DATA(arcadia/alice/iot/bulbasaur/server/testdata/turn-on-lamp-for-half-an-hour.protobuf)

DATA(arcadia/alice/iot/bulbasaur/server/testdata/scenario-create.protobuf)

TEST_CWD(alice/iot/bulbasaur/server)

SIZE(MEDIUM)

END()
