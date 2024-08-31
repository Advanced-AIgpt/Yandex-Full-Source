GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    client.go
    device.go
    experiment.go
    favorite.go
    favorite_model.go
    group.go
    household.go
    intent_state.go
    interface.go
    metrics.go
    network.go
    room.go
    scenario.go
    sharing.go
    skill.go
    stereopair.go
    template.go
    testing.go
    user.go
    user_info.go
    user_storage.go
)

GO_TEST_SRCS(
    client_test.go
    device_test.go
    experiment_test.go
    favorite_model_test.go
    favorite_test.go
    group_test.go
    household_test.go
    intent_state_test.go
    network_test.go
    room_test.go
    scenario_test.go
    sharing_test.go
    skill_test.go
    stereopair_test.go
    user_info_test.go
    user_storage_test.go
    user_test.go
)

END()

RECURSE(
    dao
    dbfiller
    schema
)

RECURSE_FOR_TESTS(gotest)
