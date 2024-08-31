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
    --id-to-string-path strings.txt
    --entity-searcher-data-path custom_entities.trie
    --entity-strings-path entity_strings.txt
    --uniq
    --trie-type TEntitySearcher
    IN
        ${ARCADIA_ROOT}/alice/vins/apps/personal_assistant/personal_assistant/config/scenarios/ar-VinsProjectfile.json
        ${AR_ENTITY_FILES}
    OUT strings.txt custom_entities.trie entity_strings.txt
)

END()
