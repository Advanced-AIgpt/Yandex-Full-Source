LIBRARY()

OWNER(
    g:alice-alarm-scenario
    g:hollywood
    g:alice
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/scenarios/alarm/proto

    library/cpp/timezone_conversion
)

SRCS(
    context.cpp
    renderer.cpp
)

END()
