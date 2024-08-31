UNITTEST_FOR(alice/hollywood/library/scenarios/news)

OWNER(
    g:hollywood
    khr2
)

PEERDIR(
    alice/hollywood/library/context
    alice/hollywood/library/framework/unittest
    alice/library/json
    alice/library/unittest
    alice/megamind/library/util
    library/cpp/testing/gmock_in_unittest
    apphost/lib/service_testing
)

SRCS(
    alice/hollywood/library/scenarios/news/render_handle_ut.cpp
)

DATA(
    arcadia/alice/hollywood/library/scenarios/news/ut_render/data/
)

END()
