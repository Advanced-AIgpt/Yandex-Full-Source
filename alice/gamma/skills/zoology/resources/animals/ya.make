OWNER(
    g-kostin
    g:alice
)

GO_LIBRARY()

RESOURCE(
    animals.json animals.json
    patterns.json patterns.json
)

SRCS(animals.go)

END()
