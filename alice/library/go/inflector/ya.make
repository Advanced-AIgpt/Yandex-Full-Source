# yo ignore:file
GO_LIBRARY()

OWNER(g:alice_iot)

IF (CGO_ENABLED)
    PEERDIR(kernel/inflectorlib/phrase/simple)
    SRCS(CGO_EXPORT inflector.cpp)
    CGO_SRCS(client.go)
    GO_TEST_SRCS(client_test.go)
ELSE()
    SRCS(stub.go)
ENDIF()

SRCS(
    const.go
    interface.go
    mock.go
    model.go
)

END()

RECURSE_FOR_TESTS(gotest)
