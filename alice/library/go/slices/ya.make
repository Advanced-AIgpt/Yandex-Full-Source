GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(dedup.go)

GO_TEST_SRCS(dedup_test.go)

END()

RECURSE(gotest)
