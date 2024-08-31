GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    analytics.go
    controller.go
    icontroller.go
)

GO_TEST_SRCS(
    analytics_test.go
    controller_test.go
)

END()

RECURSE_FOR_TESTS(gotest)
