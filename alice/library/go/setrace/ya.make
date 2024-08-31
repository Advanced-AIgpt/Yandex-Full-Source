GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    context.go
    ctxlog.go
    encoder.go
    entry_filtering_core.go
    meta.go
    round_tripper.go
)

END()

RECURSE(protos)
