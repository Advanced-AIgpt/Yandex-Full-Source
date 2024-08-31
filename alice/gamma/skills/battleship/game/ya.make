OWNER(
    g-kostin
    g:alice
)

GO_LIBRARY()

SRCS(
    context.go
    field.go
    game.go
    images.go
)

GO_TEST_SRCS(
    game_test.go
    main_test.go
)

END()

RECURSE(gotest)
