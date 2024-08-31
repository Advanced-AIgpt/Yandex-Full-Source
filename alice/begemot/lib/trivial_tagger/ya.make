LIBRARY()

OWNER(
    samoylovboris
    g:alice_quality
)

SRCS(
    config.cpp
    tagger.cpp
)

PEERDIR(
    alice/begemot/lib/trivial_tagger/proto
    alice/begemot/lib/fresh_options
    alice/begemot/lib/utils
    alice/library/json
    alice/megamind/protos/common
    library/cpp/json
    library/cpp/langs
    search/begemot/core
)

END()

RECURSE_FOR_TESTS(ut)
