GO_PROGRAM(megamind-go-example-app)

OWNER(
    alkapov
)

PEERDIR(
    alice/megamind/protos/scenarios
    alice/megamind/example_apps/go/apps
)

SRCS(main.go)

END()

RECURSE(apps)
