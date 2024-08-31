LIBRARY()

OWNER(g:megamind)

PEERDIR(
    alice/library/logger
    alice/megamind/protos/scenarios
    library/cpp/json
    library/cpp/scheme
    library/cpp/string_utils/base64
    search/begemot/rules/alice/fixlist/proto
    search/begemot/rules/alice/item_selector/proto
    search/begemot/rules/external_markup/proto
)

SRCS(
    begemot_fixlist_converter.cpp
    begemot_item_selector.cpp
    markup_converter.cpp
)

END()

RECURSE_FOR_TESTS(ut)
