GO_LIBRARY()

OWNER(g:alice)

RESOURCE(
    levels.json levels.json
    words.json words.json
)

SRCS(
    levels.go
    words.go
)

END()
