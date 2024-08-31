GO_LIBRARY()

OWNER(
    g-kostin
    g:alice
)

PEERDIR(vendor/go.uber.org/zap)

SRCS(log.go)

END()
