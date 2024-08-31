GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    action_frame.go
    action_frame_v2.go
    const.go
    discovery_frames.go
    endpoint_events_batch_frame.go
    query_frame.go
    repeat_after_me.go
    scenario_frames.go
    slots.go
    speaker_action.go
    step_actions_frame.go
    yandexio.go
)

GO_TEST_SRCS(
    action_frame_test.go
    action_frame_v2_test.go
    query_frame_test.go
    slots_test.go
)

END()

RECURSE(gotest)
