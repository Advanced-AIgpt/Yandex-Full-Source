UNION()

OWNER(
    g:alice_quality
)

SET(PERSONAL_ASSISTANT_APP_ROOT alice/vins/apps/personal_assistant/)
INCLUDE(${ARCADIA_ROOT}/alice/vins/apps/personal_assistant/entity_files.inc)

RUN_PROGRAM(
    alice/nlu/tools/build_entity_trie
    --path-to-config ${ARCADIA_ROOT}/alice/vins/apps/personal_assistant
    --paths-to-vins-files personal_assistant/config/automotive/VinsProjectfile.json
    --paths-to-vins-files personal_assistant/config/general_conversation/VinsProjectfile.json
    --paths-to-vins-files personal_assistant/config/scenarios/VinsProjectfile.json
    --paths-to-vins-files personal_assistant/config/stroka/VinsProjectfile.json
    --paths-to-vins-files personal_assistant/config/navi/VinsProjectfile.json
    --paths-to-vins-files personal_assistant/config/handcrafted/VinsProjectfile.json
    --paths-to-vins-files personal_assistant/config/internal/VinsProjectfile.json
    --paths-to-vins-files personal_assistant/config/feedback/VinsProjectfile.json 
    --id-to-string-path strings.txt
    --entity-searcher-data-path custom_entities.trie
    --entity-strings-path entity_strings.txt
    --uniq
    --trie-type TEntitySearcher
    IN
        ${ARCADIA_ROOT}/alice/vins/apps/personal_assistant/personal_assistant/config/automotive/VinsProjectfile.json
        ${ARCADIA_ROOT}/alice/vins/apps/personal_assistant/personal_assistant/config/general_conversation/VinsProjectfile.json
        ${ARCADIA_ROOT}/alice/vins/apps/personal_assistant/personal_assistant/config/scenarios/VinsProjectfile.json
        ${ARCADIA_ROOT}/alice/vins/apps/personal_assistant/personal_assistant/config/stroka/VinsProjectfile.json
        ${ARCADIA_ROOT}/alice/vins/apps/personal_assistant/personal_assistant/config/navi/VinsProjectfile.json
        ${ARCADIA_ROOT}/alice/vins/apps/personal_assistant/personal_assistant/config/handcrafted/VinsProjectfile.json
        ${ARCADIA_ROOT}/alice/vins/apps/personal_assistant/personal_assistant/config/internal/VinsProjectfile.json
        ${ARCADIA_ROOT}/alice/vins/apps/personal_assistant/personal_assistant/config/feedback/VinsProjectfile.json
        ${PERSONAL_ASSISTANT_ENTITY_FILES}
    OUT strings.txt custom_entities.trie entity_strings.txt
)

END()
