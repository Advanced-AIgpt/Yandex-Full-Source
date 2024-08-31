GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    apply_arguments.go
    device_result.go
    nlg.go
    processor.go
)

GO_TEST_SRCS(processor_test.go)

END()

RECURSE(gotest)
