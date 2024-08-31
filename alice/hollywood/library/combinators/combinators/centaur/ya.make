LIBRARY()

NO_COMPILER_WARNINGS()

OWNER(
    g:yandexdialogs2
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/base_hw_service
    alice/hollywood/library/combinators/metrics
    alice/hollywood/library/combinators/protos
    alice/hollywood/library/combinators/request
    alice/hollywood/library/combinators/combinators/centaur/main_screen
    alice/hollywood/library/combinators/combinators/centaur/teasers
    alice/hollywood/library/registry
    alice/hollywood/library/util
    alice/megamind/protos/scenarios
)

SRCS(
    centaur_teasers.cpp
    centaur_main_screen.cpp
    schedule_service.cpp
    widget_service.cpp
    teaser_service.cpp
    combinator_context_wrapper.cpp

    GLOBAL register.cpp
)

END()

RECURSE(main_screen)

RECURSE_FOR_TESTS(ut)
