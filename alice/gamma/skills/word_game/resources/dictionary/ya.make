GO_LIBRARY()

OWNER(g:alice)

RESOURCE(
    dictionary.json dictionary.json
)

PEERDIR(
    alice/gamma/skills/word_game/resources/replies
    alice/gamma/skills/word_game/resources/patterns
)

SRCS(dictionary.go)

END()
