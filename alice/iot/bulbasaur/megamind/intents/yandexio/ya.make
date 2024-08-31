GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    actions_processor.go
    arguments.go
    capability_events_processor.go
    events_batch_processor.go
    frames.go
    updates_processor.go
)

GO_TEST_SRCS(updates_processor_test.go)

END()

RECURSE(gotest)
