UNITTEST_FOR(alice/nlg/library/runtime_api)

OWNER(alexanderplat g:alice)

SRCS(
    test_env.cpp
    test_globals.cpp
    test_postprocessing.cpp
    test_range.cpp
    test_text.cpp
    test_value.cpp
)

PEERDIR(
    library/cpp/testing/unittest
)

END()
