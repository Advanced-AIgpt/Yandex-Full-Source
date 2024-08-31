LIBRARY()

OWNER(
    samoylovboris
    the0
    g:alice_quality
)

PEERDIR(
    alice/hollywood/library/base_scenario
    alice/hollywood/library/http_proxy
    alice/hollywood/library/scenarios/food/backend/proto
    alice/library/json
    library/cpp/dbg_output
    library/cpp/geo
    library/cpp/http/io
    library/cpp/json
    library/cpp/string_utils/quote
)

SRCS(
    auth.cpp
    get_address.cpp
    get_last_order_pa.cpp
    get_menu_pa.cpp
    http_utils.cpp
)

END()

RECURSE(
    proto
)
