UNION()

OWNER(
    dan-anastasev
    g:hollywood
)

FROM_SANDBOX(
    FILE
    1346147296
    OUT_NOAUTO
    video_base.txt
)

FROM_SANDBOX(
    1336407718
    OUT_NOAUTO
    bigrams_v_20200101.dssm
    bigrams_v_20200101_config.json
)

COPY_FILE(
    alice/hollywood/library/scenarios/video_recommendation/resources/sample_video_base.txt sample_video_base.txt
)

END()
