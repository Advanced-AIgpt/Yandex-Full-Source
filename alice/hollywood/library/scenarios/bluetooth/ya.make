LIBRARY()

OWNER(
    mihajlova
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/scenarios/bluetooth/nlg
    alice/hollywood/library/scenarios/bluetooth/proto
    alice/megamind/protos/common
)

SRCS(
    GLOBAL bluetooth.cpp
    bluetooth_scene.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
    it2
)
