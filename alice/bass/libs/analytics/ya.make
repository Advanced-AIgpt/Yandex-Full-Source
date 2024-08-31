LIBRARY()

OWNER(g:bass)

PEERDIR(
    alice/library/analytics/scenario
    library/cpp/string_utils/base64
)

SRCS(
    analytics.cpp
    builder.cpp
)

END()
