UNITTEST_FOR(alice/hollywood/library/scenarios/goodwin/handlers)

OWNER(
    the0
    g:megamind
)

PEERDIR(
    alice/library/frame
    alice/library/unittest
    alice/megamind/library/testing
    contrib/libs/protobuf
    apphost/lib/service_testing
)

RESOURCE(
    base_goodwin_doc.pb.txt base_goodwin_doc.pb.txt
)

SRCS(
    actions_ut.cpp
    goodwin_ut.cpp
)

END()
