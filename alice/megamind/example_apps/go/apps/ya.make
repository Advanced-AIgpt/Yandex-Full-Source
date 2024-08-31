GO_LIBRARY()

OWNER(
    alkapov
)

PEERDIR(
    alice/megamind/example_apps/go/apps/proto
    alice/megamind/protos/scenarios
)

SRCS(
    app.go
    apply_checker.go
    cards_provider.go
    commit.go
    memento.go
    stack_engine.go
    state_checker.go
)

END()
