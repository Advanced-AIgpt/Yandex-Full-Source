GO_LIBRARY()

OWNER(
    g-kostin
    g:alice
)

PEERDIR(vendor/golang.org/x/xerrors)

SRCS(
    context.go
    game.go
    state.go
)

GO_TEST_SRCS(
    game_test.go
    main_test.go
    state_test.go
)

END()

RECURSE(gotest)
