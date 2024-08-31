GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    controller.go
    interface.go
    model.go
)

GO_TEST_SRCS(
    controller_test.go
    scenario_test.go
    suite_test.go
)

END()

RECURSE(
    gotest
    timetable
)
