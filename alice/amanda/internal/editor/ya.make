GO_LIBRARY()

OWNER(
    g:amanda
    alkapov
)

PEERDIR(
    alice/amanda/internal/session

    library/go/core/resource

    vendor/go.uber.org/zap
)

RESOURCE(
    templates/device_state_editor.html templates/device_state_editor.html
)

SRCS(
    editor.go
)

END()
