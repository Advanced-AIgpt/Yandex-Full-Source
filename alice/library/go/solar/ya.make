GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(sun.go)

GO_TEST_SRCS(sun_test.go)

END()

RECURSE(gotest)
