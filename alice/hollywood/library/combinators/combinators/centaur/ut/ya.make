UNITTEST_FOR(alice/hollywood/library/combinators/combinators/centaur)

OWNER(
    nkodosov
)

PEERDIR(
    alice/hollywood/library/testing
    alice/library/unittest
    library/cpp/testing/gmock_in_unittest
    apphost/lib/service_testing
)

SRCS(
    alice/hollywood/library/combinators/combinators/centaur/centaur_teasers_ut.cpp
    alice/hollywood/library/combinators/combinators/centaur/centaur_main_screen_ut.cpp
)

DATA(
    arcadia/alice/hollywood/library/combinators/combinators/centaur/ut/data/
)

END()
