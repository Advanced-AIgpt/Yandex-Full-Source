
UNION()

OWNER(
    moath-alali
    g:alice_quality
)

INCLUDE(${ARCADIA_ROOT}/alice/nlu/data/ar/entities/entity_files.inc)

RUN_PROGRAM(
    alice/nlu/tools/build_entity_trie
    --path-to-config ${ARCADIA_ROOT}/alice/vins/apps/personal_assistant
    --paths-to-vins-files personal_assistant/config/scenarios/ar-VinsProjectfile.json
    --uniq
    IN
        ${ARCADIA_ROOT}/alice/vins/apps/personal_assistant/personal_assistant/config/scenarios/ar-VinsProjectfile.json
        ${AR_ENTITY_FILES}
    STDOUT custom_entities.trie
)

END()
