LIBRARY()

OWNER(d-dima)

PEERDIR (
    alice/hollywood/library/framework
    alice/hollywood/library/scenarios/get_date/proto
    alice/library/geo
    alice/library/sys_datetime
    alice/megamind/protos/common
)

SRCS(
    slot_utils.cpp
    calendar_utils.cpp
)

END()
