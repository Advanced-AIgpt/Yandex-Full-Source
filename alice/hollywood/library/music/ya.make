LIBRARY()

OWNER(
    sparkle
    g:hollywood
    g:alice
)

PEERDIR(
    alice/hollywood/library/base_scenario
    alice/hollywood/library/http_proxy
    alice/hollywood/library/request
    alice/library/json
    alice/library/smallgeo
    alice/library/websearch
)

SRCS(
    fm_radio.cpp
    fm_radio_resources.cpp
    music_catalog.cpp
    music_resources.cpp
    create_search_request.cpp
)

END()
