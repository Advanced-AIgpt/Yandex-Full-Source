OWNER(
    g-kostin
    g:alice
)

GO_LIBRARY()

SRCS(
    encoder.go
    sensors.go
)

GO_TEST_SRCS(encoder_test.go)

END()

RECURSE(gotest)
