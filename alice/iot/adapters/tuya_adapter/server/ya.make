GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    adapter_handlers.go
    client_handlers.go
    common.go
    ir_handlers.go
    mobile_handlers.go
    pulsar_handlers.go
    server.go
    service_handlers.go
    steelix_auth_policy.go
)

END()
