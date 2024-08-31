GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    blackbox.go
    csrf.go
    logging.go
    multiauth.go
    recorder.go
    recoverer.go
    servicehost.go
    splitter.go
    timestamper.go
    tvm.go
    user.go
    user_extractor.go
)

GO_TEST_SRCS(splitter_test.go)

END()

RECURSE(gotest)
