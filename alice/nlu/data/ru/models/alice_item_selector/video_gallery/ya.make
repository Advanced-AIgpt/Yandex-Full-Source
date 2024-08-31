OWNER(
    volobuev
    g:alice_quality
)
UNION()

FROM_SANDBOX(
    1188668356
    OUT
    tagger/tagger.data/personal_assistant.scenarios.video_play/model.mmap
    tagger/tagger.data/personal_assistant.scenarios.video_play/model_description
)

FROM_SANDBOX(
    1352645332
    OUT
    model/model.cbm
    model/spec.json
)

FROM_SANDBOX(
    1773250078
    OUT_NOAUTO
    model_description
    model_description.json
    model.mmap
    model.pb
    trainable_embeddings.json
)

END()
