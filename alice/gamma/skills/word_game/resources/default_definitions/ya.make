GO_LIBRARY(defaultdefinitions)

OWNER(g:alice)

RESOURCE(
    default_definitions.json default_definitions.json
)

PEERDIR(
    alice/gamma/skills/word_game/resources/replies
    alice/gamma/skills/word_game/resources/patterns
)

SRCS(default_definitions.go)

END()
