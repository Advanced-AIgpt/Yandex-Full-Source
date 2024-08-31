OWNER(
    g-kostin
    g:alice
)

GO_PROGRAM()

PEERDIR(
    alice/gamma/sdk/golang
    alice/gamma/sdk/golang/testing

    alice/gamma/skills/song_search/resources/buttons
    alice/gamma/skills/song_search/resources/patterns
    alice/gamma/skills/song_search/resources/replies
)

SRCS(
    main.go
    skill.go
)

END()

RECURSE_FOR_TESTS(
    api
    game
    resources
)
