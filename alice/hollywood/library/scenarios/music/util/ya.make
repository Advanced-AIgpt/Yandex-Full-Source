LIBRARY()

OWNER(
    sparkle
    g:hollywood
    g:alice
)

PEERDIR(
    alice/hollywood/library/capability_wrapper
    alice/hollywood/library/crypto
    alice/hollywood/library/http_proxy
    alice/hollywood/library/music
    alice/hollywood/library/response
    alice/hollywood/library/scenarios/music/proto
    alice/library/json
    alice/library/music
    alice/library/util
    alice/megamind/protos/guest
    library/cpp/string_utils/base64
)

JOIN_SRCS(
    all.cpp
    music_proxy_request.cpp
    onboarding.cpp
    util.cpp
)

END()

RECURSE_FOR_TESTS(ut)
