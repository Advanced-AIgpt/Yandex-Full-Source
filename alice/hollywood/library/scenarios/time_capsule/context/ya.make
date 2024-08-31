LIBRARY()

OWNER(
    g:alice-time-capsule-scenario
    g:hollywood
    g:alice
)

PEERDIR(
    alice/hollywood/library/response
    alice/hollywood/library/scenarios/time_capsule/proto

    library/cpp/timezone_conversion
)

SRCS(
    context.cpp
)

END()
