GO_PROGRAM()

OWNER(
    g-kostin
    g:alice
)

PEERDIR(
    alice/gamma/sdk/golang
    alice/gamma/sdk/golang/testing
    alice/gamma/skills/cheat_game/game
    alice/gamma/skills/cheat_game/resources/buttons
    alice/gamma/skills/cheat_game/resources/replies
    alice/gamma/skills/cheat_game/resources/patterns
)

SRCS(
    main.go
    skill.go
)

GO_TEST_SRCS(skill_test.go)

END()

RECURSE(
    game
    gotest
    resources
)

RECURSE_FOR_TESTS(
    game
    resources
    gotest
)
