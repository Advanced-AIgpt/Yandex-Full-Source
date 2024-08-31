UNION()

OWNER(
    g:alice_quality
)

SET(NAVI_APP_ROOT alice/vins/apps/navi/)
INCLUDE(${ARCADIA_ROOT}/alice/vins/apps/navi/entity_files.inc)

RUN_PROGRAM(
    alice/nlu/tools/build_entity_trie
    --path-to-config ${ARCADIA_ROOT}/alice/vins/apps/navi
    --paths-to-vins-files navi_app/config/navi_ru/VinsProjectfile.json
    --uniq
    IN
        ${ARCADIA_ROOT}/alice/vins/apps/navi/navi_app/config/navi_ru/VinsProjectfile.json
        ${NAVI_ENTITY_FILES}
    STDOUT custom_entities.trie
)

END()
