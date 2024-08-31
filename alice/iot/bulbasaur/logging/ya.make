GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    masking.go
    utils.go
)

GO_TEST_SRCS(masking_test.go)

END()

RECURSE(
    doublelog
    gotest
)
