OWNER(
    samoylovboris
    g:alice_quality
)

UNION()

FROM_SANDBOX(1113892352 OUT_NOAUTO authorize_video_provider.tsv)
FROM_SANDBOX(1113894721 OUT_NOAUTO goto_video_screen.tsv)
FROM_SANDBOX(1113896575 OUT_NOAUTO open_current_video.tsv)
FROM_SANDBOX(1113899487 OUT_NOAUTO payment_confirmed.tsv)

END()
