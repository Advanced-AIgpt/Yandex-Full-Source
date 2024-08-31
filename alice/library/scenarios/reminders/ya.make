LIBRARY()

OWNER(
    petrk
)

PEERDIR(
    alice/library/frame
    alice/library/proto
    alice/megamind/protos/common
    alice/megamind/protos/scenarios
    alice/megamind/protos/speechkit
    alice/memento/proto
    alice/protos/api/matrix
    alice/protos/data/scenario/reminders
    infra/libs/outcome
    library/cpp/protobuf/interop
    library/cpp/timezone_conversion
)

SRCS(
    api.cpp
    common.cpp
    datetime.cpp
    memento.cpp
    schedule.cpp
)

GENERATE_ENUM_SERIALIZATION(api.h)

END()

RECURSE_FOR_TESTS(ut)
