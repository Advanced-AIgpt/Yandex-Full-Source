GO_TEST_FOR(alice/library/go/logbroker)

OWNER(g:alice_iot)

# recipe for testing logbroker integration locally

INCLUDE(${ARCADIA_ROOT}/kikimr/public/tools/lbk_recipe/recipe_stable.inc)

SIZE(MEDIUM)

ENV(LOGBROKER_CREATE_TOPICS=default-topic,writer-topic,pool-topic)

END()
