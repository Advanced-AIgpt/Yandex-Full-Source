GO_LIBRARY()

OWNER(
    g:amanda
    alkapov
)

PEERDIR(
    vendor/github.com/aws/aws-sdk-go/aws
    vendor/github.com/aws/aws-sdk-go/aws/awserr
    vendor/github.com/aws/aws-sdk-go/aws/session
    vendor/github.com/aws/aws-sdk-go/service/s3
    vendor/github.com/aws/aws-sdk-go/service/s3/s3manager
)

SRCS(
    mds.go
)

END()
