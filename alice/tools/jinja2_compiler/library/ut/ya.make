UNITTEST_FOR(alice/tools/jinja2_compiler/library)

OWNER(
    d-dima
)

PEERDIR(
    library/cpp/testing/gmock_in_unittest
)

DATA(
    arcadia/alice/tools/jinja2_compiler/library/ut/test.proto
)

SRCS(
    jinja2_compiler_ut.cpp
)

END()
