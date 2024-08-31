GO_LIBRARY()

OWNER(
    g-kostin
    g:alice
)

PEERDIR(vendor/golang.org/x/xerrors)

SRCS(
    compile.go
    lexer.go
    parser.go
)

GO_TEST_SRCS(
    compile_test.go
    lexer_test.go
    parser_test.go
)

END()

RECURSE(gotest)

RECURSE_FOR_TESTS(gotest)
