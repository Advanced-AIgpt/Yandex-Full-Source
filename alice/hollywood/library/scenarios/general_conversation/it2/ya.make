PY3TEST()

OWNER(
    g:alice_boltalka
    g:hollywood
)

SIZE(MEDIUM)

FORK_SUBTESTS()

SET(HOLLYWOOD_SHARD general_conversation)
ENV(HOLLYWOOD_SHARD=general_conversation)
INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it2/common.inc)

PEERDIR(
    alice/hollywood/library/scenarios/general_conversation/proto
)

TEST_SRCS(
    conftest.py
    tests_base.py
    tests_heavy_base.py
    test_cases_base_all_apps.py
    test_cases_facts_crosspromo.py
    test_cases_generative_tale.py
    test_cases_lets_discuss_movie.py
    test_cases_proactivity.py
)

DATA(
    arcadia/alice/hollywood/library/scenarios/general_conversation/it2/tests_base
    arcadia/alice/hollywood/library/scenarios/general_conversation/it2/tests_heavy_base
    arcadia/alice/hollywood/library/scenarios/general_conversation/it2/test_cases_base_all_apps
    arcadia/alice/hollywood/library/scenarios/general_conversation/it2/test_cases_facts_crosspromo
    arcadia/alice/hollywood/library/scenarios/general_conversation/it2/test_cases_generative_tale
    arcadia/alice/hollywood/library/scenarios/general_conversation/it2/test_cases_lets_discuss_movie
    arcadia/alice/hollywood/library/scenarios/general_conversation/it2/test_cases_proactivity
)

REQUIREMENTS(ram:32)

END()
