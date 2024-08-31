GO_LIBRARY()

OWNER(
    g-kostin
    g:alice
)

SRCS(adapter.go)

GO_TEST_SRCS(ydb_test.go)

END()

RECURSE(gotest)

RECURSE_FOR_TESTS(gotest)
