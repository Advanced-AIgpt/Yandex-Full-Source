GO_LIBRARY()

OWNER(g:alice)

PEERDIR(
    alice/gamma/skills/word_game/resources/replies
    alice/gamma/skills/word_game/resources/patterns
)

RESOURCE(
    words.json words.json
)

SRCS(words.go)

END()
