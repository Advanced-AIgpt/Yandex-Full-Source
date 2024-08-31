LIBRARY()

OWNER(
    caesium
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/http_proxy
    alice/hollywood/library/request
    alice/hollywood/library/scenarios/order/nlg
    alice/hollywood/library/scenarios/order/proto
    alice/protos/data/scenario/order
)

SRCS(
    GLOBAL order.cpp
    order_algorithm.cpp
    order_relevant_error_scene.cpp
    order_scene.cpp
    order_status_notification_scene.cpp
    response_convertor.cpp
    test_json_constants.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
    it2
)
