PY3TEST()

OWNER(
    mamay-igor
    abc:zenfront
)

FORK_SUBTESTS()
SPLIT_FACTOR(2) # В зависимости от количества тестов у my_scenario степень параллелизма можно увеличивать

SIZE(MEDIUM)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it2/common.inc)

TEST_SRCS(
    tests.py
)

REQUIREMENTS(ram:32)

END()
