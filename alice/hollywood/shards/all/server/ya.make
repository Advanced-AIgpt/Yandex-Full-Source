PROGRAM(hollywood_server)

ALLOCATOR(HU)

OWNER(
    akhruslan
    g:hollywood
)

SRCS(
    main.cpp
)

PEERDIR(
    alice/hollywood/library/hollywood_runner
    alice/hollywood/shards/all/hw_services
    alice/hollywood/shards/all/scenarios
)

END()
