GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    changelog.go
    client.go
    colors.go
    const.go
    context.go
    custom_control.go
    encryptor.go
    errors.go
    ir_model.go
    metrics.go
    model.go
    pulsar_client.go
    pulsar_status.go
    scenes.go
    scene_pools.go
    token.go
    utils.go
    validation.go
)

GO_TEST_SRCS(
    const_test.go
    changelog_test.go
    custom_control_test.go
    ir_model_test.go
    model_test.go
    pulsar_status_test.go
    scene_pools_test.go
    token_test.go
    utils_test.go
    validation_test.go
)

END()

RECURSE_FOR_TESTS(gotest)
