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
    alice/hollywood/library/registry
    alice/hollywood/library/util
    alice/library/proto
    alice/megamind/protos/common
    alice/megamind/protos/scenarios
    alice/memento/proto
    alice/protos/api/renderer
    alice/protos/data/scenario
)

SRCS(
    prepare_teasers.cpp
    prepare_teaser_settings_screen.cpp
    set_teaser_settings.cpp
)

END()