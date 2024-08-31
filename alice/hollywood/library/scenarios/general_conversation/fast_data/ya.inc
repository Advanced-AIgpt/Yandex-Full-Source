UNION()

OWNER(
    deemonasd
    g:hollywood
)

RUN_PROGRAM(
    alice/hollywood/library/scenarios/general_conversation/fast_data/util/convert_fixlist
    alice/begemot/lib/fixlist_index/data/ru/gc_response_banlist.yaml
    alice/hollywood/library/scenarios/general_conversation/fast_data/gc_request_banlist_small.yaml
    alice/hollywood/library/scenarios/general_conversation/fast_data/gc_request_banlist_tale.yaml
    alice/hollywood/library/scenarios/general_conversation/fast_data/gif_response_url_banlist.yaml
    alice/hollywood/library/scenarios/general_conversation/fast_data/facts_crosspromo_response_banlist.yaml
    alice/hollywood/library/scenarios/general_conversation/fast_data/set_of_linear_coefficients.yaml
    general_conversation.pb.txt
    IN alice/begemot/lib/fixlist_index/data/ru/gc_response_banlist.yaml
    IN alice/hollywood/library/scenarios/general_conversation/fast_data/gc_request_banlist_small.yaml
    IN alice/hollywood/library/scenarios/general_conversation/fast_data/gc_request_banlist_tale.yaml
    IN alice/hollywood/library/scenarios/general_conversation/fast_data/gif_response_url_banlist.yaml
    IN alice/hollywood/library/scenarios/general_conversation/fast_data/facts_crosspromo_response_banlist.yaml
    IN alice/hollywood/library/scenarios/general_conversation/fast_data/set_of_linear_coefficients.yaml
    OUT_NOAUTO general_conversation.pb.txt
)

RUN_PROGRAM(
    alice/hollywood/convert_proto
    --proto TGeneralConversationFastDataProto
    --to-binary
    general_conversation.pb.txt
    general_conversation.pb
    IN general_conversation.pb.txt
    OUT general_conversation.pb
)

RUN_PROGRAM(
    alice/hollywood/convert_proto
    --proto TGeneralConversationProactivityFastDataProto
    --to-binary
    alice/hollywood/library/scenarios/general_conversation/fast_data/general_conversation_proactivity.pb.txt
    general_conversation_proactivity.pb
    IN alice/hollywood/library/scenarios/general_conversation/fast_data/general_conversation_proactivity.pb.txt
    OUT general_conversation_proactivity.pb
)

END()
