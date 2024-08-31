PY2_LIBRARY()

OWNER(g:alice)

PEERDIR(
    alice/vins/core
    alice/vins/sdk
    alice/vins/apps/personal_assistant
)

RESOURCE_FILES(
    gc_skill/config/general_conversation.json
    gc_skill/config/session_start.nlg
    gc_skill/config/general_conversation.nlg
    gc_skill/config/Microintents.json
    gc_skill/config/topics.yaml
    gc_skill/config/general_conversation_dummy.json
    gc_skill/config/skill.microintents.yaml
    gc_skill/config/Vinsfile.json
)

# symlinks gc_skill/config/alice.microintents.yaml and gc_skill/config/blogger_name.txt
RESOURCE(
    alice/vins/apps/personal_assistant/personal_assistant/config/handcrafted/config.microintents.yaml resfs/file/gc_skill/config/alice.microintents.yaml
    alice/vins/apps/personal_assistant/personal_assistant/config/nlu_templates/blogger_name.txt       resfs/file/gc_skill/config/blogger_name.txt
)

PY_SRCS(
    TOP_LEVEL
    gc_skill/app.py
    gc_skill/__init__.py
)

END()

RECURSE_FOR_TESTS(ut)
