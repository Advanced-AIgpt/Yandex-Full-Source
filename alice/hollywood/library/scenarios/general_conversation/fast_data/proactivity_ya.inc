UNION()

OWNER(
    deemonasd
    g:hollywood
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
