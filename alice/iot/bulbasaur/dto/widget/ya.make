GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    capability.go
    const.go
    error.go
    lighting.go
    scenario.go
    sorting.go
    speakers.go
)

GO_TEST_SRCS(
    scenario_test.go
    sorting_test.go
)

END()

RECURSE(gotest)
