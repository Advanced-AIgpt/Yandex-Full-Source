UNION()

OWNER(
    dan-anastasev
    g:hollywood
)

INCLUDE(${ARCADIA_ROOT}/alice/hollywood/library/scenarios/suggesters/movie_akinator/resources/resource.make)

FROM_SANDBOX(
    ${CLUSTERED_MOVIES_RESOURCE_ID}
    OUT_NOAUTO
    clustered_movies.json
)

FROM_SANDBOX(
    FILE
    1674688331

    OUT_NOAUTO
    known_movies.json
)

END()
