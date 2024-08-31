OWNER(
    g-kostin
    g:alice
)

GO_LIBRARY()

PEERDIR(
    vendor/golang.org/x/xerrors
)

SRCS(
    game.go
    context.go
)

GO_TEST_SRCS(
    main_test.go
    game_test.go
)

END()

RECURSE(
    gotest
)
