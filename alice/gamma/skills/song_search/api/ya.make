OWNER(
    g-kostin
    g:alice
)

GO_LIBRARY()

PEERDIR(
    vendor/golang.org/x/xerrors
)

SRCS(
    api.go
    search.go
    dummy_search.go
)

GO_TEST_SRCS(
    search_test.go
)


END()

RECURSE_FOR_TESTS(
    gotest
)

