GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    analytics.go
    const.go
    controller.go
    execution_plan.go
    execution_planner.go
    execution_results.go
    icontroller.go
    model.go
    retries.go
)

GO_TEST_SRCS(
    analytics_test.go
    controller_test.go
    execution_planner_test.go
    retries_test.go
    suite_test.go
)

GO_XTEST_SRCS(model_test.go)

END()

RECURSE_FOR_TESTS(gotest)
