LIBRARY()

OWNER(g:bass)

PEERDIR(
    contrib/libs/aws-sdk-cpp/aws-cpp-sdk-s3
)

SRCS(
    s3.cpp
)

END()
