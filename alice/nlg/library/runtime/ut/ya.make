UNITTEST_FOR(alice/nlg/library/runtime)

OWNER(alexanderplat g:alice)

SRCS(
    test_builtins.cpp
    test_coverage.cpp
    test_emoji.cpp
    test_helpers.cpp
    test_inflector.cpp
    test_operators.cpp
)

PEERDIR(
    alice/library/sys_datetime
    library/cpp/testing/unittest
)

RESOURCE(
    alice/nlg/library/runtime/ut/data/datetime_raw_examples.json /datetime_raw_examples.json
)

END()
