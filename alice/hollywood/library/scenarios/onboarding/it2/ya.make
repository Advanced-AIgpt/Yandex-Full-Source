PY3TEST()

OWNER(
    karina-usm
    vitamin-ca
    alexeybabenko
    g:hollywood
    g:alice
    g:home
)

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it2/common.inc)

TEST_SRCS(
  tests_greetings.py
  tests_what_can_you_do.py
)

REQUIREMENTS(ram:32)

PEERDIR(
    alice/hollywood/library/scenarios/onboarding/proto
    dj/services/alisa_skills/server/proto/client
)

DATA(
    arcadia/alice/hollywood/library/scenarios/onboarding/it2/tests_greetings
    arcadia/alice/hollywood/library/scenarios/onboarding/it2/tests_what_can_you_do
)

END()
