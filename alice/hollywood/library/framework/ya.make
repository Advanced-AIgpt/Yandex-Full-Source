LIBRARY()

OWNER(
    d-dima
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/framework/proto
    alice/hollywood/library/framework/core
    alice/megamind/protos/scenarios
)

SRCS(
    framework.cpp
    framework_migration.cpp
)

END()

RECURSE(
    helpers
)

RECURSE_FOR_TESTS(
    ut
)
