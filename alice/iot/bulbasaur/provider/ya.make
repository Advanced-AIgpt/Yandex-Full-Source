GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    auth_policy.go
    callback_metrics.go
    const.go
    errors.go
    factory.go
    imetrics.go
    interface.go
    jsonrpc_provider.go
    metrics.go
    mock.go
    model.go
    normalize_result.go
    quasar.go
    rest_provider.go
    sber.go
    tuya.go
    yandexio.go
)

GO_TEST_SRCS(
    errors_test.go
    jsonrpc_provider_test.go
    normalize_result_test.go
)

END()

RECURSE_FOR_TESTS(gotest)
