GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    apply_arguments.go
    directives.go
    nlg.go
    processor.go
)

GO_TEST_SRCS(
    nlg_test.go
    processor_test.go
)

END()

RECURSE(gotest)
