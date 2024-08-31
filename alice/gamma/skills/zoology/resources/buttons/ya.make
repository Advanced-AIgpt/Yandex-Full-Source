OWNER(
    g-kostin
    g:alice
)

GO_LIBRARY()

SRCS(buttons.go)

GO_TEST_SRCS(buttons_test.go)

END()

RECURSE(gotest)
