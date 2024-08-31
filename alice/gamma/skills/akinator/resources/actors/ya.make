GO_LIBRARY()

OWNER(g:alice)

PEERDIR(
    alice/gamma/skills/akinator/resources/replies
    alice/gamma/skills/akinator/resources/patterns
)

RESOURCE(
    actorsData.json actorsData.json
    actorsPatterns.json actorsPatterns.json
)

SRCS(actors.go)

END()
