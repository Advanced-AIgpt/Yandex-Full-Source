GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    const.go
    context.go
    experiment.go
    experiments.go
    interface.go
)

END()

RECURSE(
    dbexpmanager
    gotest
)
