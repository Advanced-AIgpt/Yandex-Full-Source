LIBRARY()

OWNER(g:voicetech-infra)

SRCS(
    apphost_http_request_signer.cpp
    constants.cpp

    GLOBAL init_crypto.cpp
)

PEERDIR(
    apphost/lib/proto_answers

    contrib/libs/aws-sdk-cpp/aws-cpp-sdk-s3
)

END()

RECURSE_FOR_TESTS(ut)
