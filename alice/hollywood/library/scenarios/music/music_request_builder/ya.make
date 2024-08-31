LIBRARY()

OWNER(
    vitvlkv
    g:hollywood
    g:alice
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/http_proxy
    alice/hollywood/library/scenarios/music/biometry
    alice/hollywood/library/scenarios/music/proto
    alice/library/logger
    library/cpp/svnversion
)

SRCS(
    music_request_builder.cpp
    music_request_mode.cpp
)

GENERATE_ENUM_SERIALIZATION(music_request_mode.h)

END()

RECURSE_FOR_TESTS(ut)
