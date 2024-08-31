GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    alice4business.go
    blackbox.go
    experiments.go
    logging.go
    metrics.go
    recoverer.go
    request_id.go
    request_source.go
    setrace.go
    timestamper.go
    tvm.go
    user_agent.go
)

END()

RECURSE(gotest)
