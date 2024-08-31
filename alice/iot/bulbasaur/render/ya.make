GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    interface.go
    renderer.go
    testing.go
)

GO_TEST_SRCS(renderer_test.go)

END()

RECURSE_FOR_TESTS(gotest)
