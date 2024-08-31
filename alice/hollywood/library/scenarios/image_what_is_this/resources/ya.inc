UNION()

OWNER(
    polushkin
    g:cv-dev
)

FROM_SANDBOX(FILE 916112353 OUT_NOAUTO swear.txt)
FROM_SANDBOX(FILE 2313852550 OUT_NOAUTO bad.txt)

COPY_FILE(
    alice/hollywood/library/scenarios/image_what_is_this/resources/entity_search_jokes.json entity_search_jokes.json
)

END()
