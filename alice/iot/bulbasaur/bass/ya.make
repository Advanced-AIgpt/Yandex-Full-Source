GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    client.go
    const.go
    interface.go
    mock.go
    model.go
    semantic_frames.go
)

GO_TEST_SRCS(semantic_frames_test.go)

END()

RECURSE_FOR_TESTS(gotest)
