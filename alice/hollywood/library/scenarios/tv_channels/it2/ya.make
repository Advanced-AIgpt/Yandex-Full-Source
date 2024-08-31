PY3TEST()

OWNER(
    hellodima
    g:smarttv
)

SIZE(MEDIUM)

FORK_SUBTESTS()
SPLIT_FACTOR(2)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/python/testing/it2/common.inc)

TEST_SRCS(
    test_channels.py
)

DATA(
    # В этой директории будут храниться файлы реквестов и ответов источников (стабы)
)

REQUIREMENTS(ram:9)

END()

