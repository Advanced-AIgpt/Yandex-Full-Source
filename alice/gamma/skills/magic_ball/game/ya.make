OWNER(
    g-kostin
    g:alice
)

GO_LIBRARY()

SRCS(
    context.go
    game.go
)

GO_TEST_SRCS(
    game_test.go
    main_test.go
)

END()

RECURSE(gotest)
