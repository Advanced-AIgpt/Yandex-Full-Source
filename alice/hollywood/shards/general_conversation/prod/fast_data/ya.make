UNION()

OWNER(
    deemonasd
    g:hollywood
)

FILES(.svninfo)

PEERDIR(
    alice/hollywood/shards/general_conversation/prod/fast_data/general_conversation
    alice/hollywood/shards/general_conversation/prod/fast_data/general_conversation_proactivity
)

END()

RECURSE(
    general_conversation
    general_conversation_proactivity
)
