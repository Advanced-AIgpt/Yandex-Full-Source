GO_LIBRARY()

OWNER(
    g:amanda
    alkapov
)

PEERDIR(
    alice/amanda/pkg/divrenderer
    alice/amanda/pkg/divrenderer/mds
    alice/amanda/pkg/divrenderer/rotor

    vendor/github.com/aws/aws-sdk-go/aws
    vendor/github.com/aws/aws-sdk-go/aws/credentials
)

SRCS(
    divrenderer.go
)

END()
