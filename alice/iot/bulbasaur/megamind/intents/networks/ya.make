GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    apply_arguments.go
    const.go
    delete_networks_processor.go
    directives.go
    frames.go
    restore_networks_processor.go
    save_networks_processor.go
)

END()
