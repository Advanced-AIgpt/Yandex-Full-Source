UNION()

OWNER(
    g:alice_quality
)

SET(CRM_BOT_APP_ROOT alice/vins/apps/crm_bot/)
INCLUDE(${ARCADIA_ROOT}/alice/vins/apps/crm_bot/entity_files.inc)

RUN_PROGRAM(
    alice/nlu/tools/build_entity_trie
    --path-to-config ${ARCADIA_ROOT}/alice/vins/apps/crm_bot
    --paths-to-vins-files crm_bot/config/scenarios/VinsProjectfile.json
    --uniq
    IN
        ${CRM_BOT_ENTITY_FILES}
        ${ARCADIA_ROOT}/alice/vins/apps/crm_bot/crm_bot/config/scenarios/VinsProjectfile.json
    STDOUT custom_entities.trie
)

END()
