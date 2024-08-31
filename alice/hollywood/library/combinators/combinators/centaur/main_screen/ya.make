LIBRARY()

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
    add_widget_from_gallery.cpp
    prepare_div_patch.cpp
    prepare_main_screen.cpp
    prepare_widget_gallery.cpp
)

END()
