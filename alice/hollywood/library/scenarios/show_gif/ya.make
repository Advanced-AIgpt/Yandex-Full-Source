LIBRARY()

OWNER(
    jan-fazli
    g:alice
)

PEERDIR(
    alice/hollywood/library/base_scenario
    alice/hollywood/library/gif_card
    alice/hollywood/library/gif_card/proto
    alice/hollywood/library/global_context
    alice/hollywood/library/nlg
    alice/hollywood/library/registry
    alice/hollywood/library/request
    alice/hollywood/library/resources
    alice/hollywood/library/response
    alice/hollywood/library/scenarios/show_gif/nlg
    alice/library/proto
    library/cpp/json
)

SRCS(
    GLOBAL show_gif.cpp
    show_gif_resources.cpp
)

END()

RECURSE_FOR_TESTS(
    it
)
