LIBRARY()

OWNER(g:alice)

PEERDIR(
    alice/library/client
    alice/library/frame
    alice/library/url_builder
    alice/megamind/protos/scenarios
    dict/dictutil
    library/cpp/cgiparam
    library/cpp/json/writer
    library/cpp/scheme
)

SRCS(
    answer.cpp
    defs.cpp
    catalog.cpp
    common_special_playlists.cpp
    fairytale_linear_albums.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
