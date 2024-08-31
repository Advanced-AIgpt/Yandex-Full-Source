GO_LIBRARY()

OWNER(g:alice_iot)

SRCS(
    buckets.go
    counter_mock.go
    middleware.go
    perf_metrics.go
    pgdb_metrics.go
    round_tripper.go
    route_signals.go
    route_signals_mock.go
    timer_mock.go
    version_registry.go
    ydb_metrics.go
)

GO_TEST_SRCS(buckets_test.go)

END()

RECURSE(gotest)
