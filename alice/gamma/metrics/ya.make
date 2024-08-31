OWNER(
    g-kostin
    g:alice
)

GO_LIBRARY()

SRCS(
    config.go
    registry.go
    sensors.go
    storage.go
)

GO_TEST_SRCS(
    registry_test.go
    storage_test.go
)

END()

RECURSE(
    generic
    gotest
    solomon
)
