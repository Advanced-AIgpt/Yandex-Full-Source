OWNER(
    deemonasd
    g:alice_boltalka
    g:hollywood
)

UNION()

FROM_SANDBOX(
    1372767290

    RENAME
    gc_reply_embedder
    OUT_NOAUTO
    embedders/nlu_search_model/embedder

    RENAME
    gc_reply_embedder_config.json
    OUT_NOAUTO
    embedders/nlu_search_model/config.json
)

FROM_SANDBOX(
    1480703010

    RENAME
    embedder
    OUT_NOAUTO
    embedders/nlu_search_context_model/embedder

    RENAME
    config.json
    OUT_NOAUTO
    embedders/nlu_search_context_model/config.json
)

FROM_SANDBOX(
    1410783382

    RENAME
    dssm.pb
    OUT_NOAUTO
    rankers/lstm256/dssm.pb

    RENAME
    dssm_config.json
    OUT_NOAUTO
    rankers/lstm256/dssm_config.json

    RENAME
    ranker_config.json
    OUT_NOAUTO
    rankers/lstm256/ranker_config.json
)

FROM_SANDBOX(
    FILE
    1527379334

    RENAME
    gifs.json
    OUT_NOAUTO
    emoji/gifs.json
)

FROM_SANDBOX(
    FILE
    1860053722

    RENAME
    led_images.json
    OUT_NOAUTO
    emoji/led_images.json
)

FROM_SANDBOX(
    1483269540

    RENAME
    model.pb
    OUT_NOAUTO
    emoji/model.pb

    RENAME
    model_config.json
    OUT_NOAUTO
    emoji/model_config.json

    RENAME
    emoji_config.json
    OUT_NOAUTO
    emoji/emoji_config.json
)

FROM_SANDBOX(
    1580740644

    OUT_NOAUTO
    onto_to_kp_id_mapping.json
)

FROM_SANDBOX(
    FILE
    1589752509

    OUT_NOAUTO
    unclassified_gifs.json
)

FROM_SANDBOX(
    FILE
    1674688331

    OUT_NOAUTO
    known_movies.json
)

FROM_SANDBOX(
    FILE
    1727002896

    OUT_NOAUTO
    movies_to_discuss.json
)

FROM_SANDBOX(
    1662257091

    OUT_NOAUTO
    crosspromo_general_conversation.json
)

FROM_SANDBOX(
    1725737748

    OUT_NOAUTO
    crosspromo_general_conversation_filtered.json
)

COPY_FILE(
    alice/library/handcrafted_data/ru/microintents.yaml ru_microintents.yaml
)

COPY_FILE(
    alice/library/handcrafted_data/ru/dummy_microintents.yaml ru_dummy_microintents.yaml
)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/scenarios/suggesters/movie_akinator/resources/resource.make)

FROM_SANDBOX(
    ${CLUSTERED_MOVIES_RESOURCE_ID}
    OUT_NOAUTO
    clustered_movies.json
)

END()
