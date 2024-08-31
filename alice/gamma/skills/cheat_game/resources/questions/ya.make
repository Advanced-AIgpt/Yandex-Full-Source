GO_LIBRARY()

OWNER(
    g-kostin
    g:alice
)

PEERDIR(
    alice/gamma/skills/cheat_game/resources/replies
    alice/gamma/skills/cheat_game/resources/patterns
)

RESOURCE(
    facts.json facts.json
)

SRCS(questions.go)

END()
