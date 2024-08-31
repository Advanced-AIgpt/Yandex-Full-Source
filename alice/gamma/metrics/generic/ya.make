OWNER(
    g-kostin
    g:alice
)

GO_LIBRARY()

SRCS(
    buckets.go
    counter.go
    gauge.go
    histogram.go
    timer.go
)

GO_TEST_SRCS(
    counter_test.go
    gauge_test.go
    buckets_test.go
    histogram_test.go
    timer_test.go
)

END()

RECURSE(gotest)
