GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    calculator.go
    jitter.go
    timetable.go
)

GO_TEST_SRCS(
    calculator_test.go
    jitter_test.go
)

END()

RECURSE(gotest)
