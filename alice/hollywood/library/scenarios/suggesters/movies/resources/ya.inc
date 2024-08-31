UNION()

OWNER(
    dan-anastasev
    g:hollywood
)

COPY_FILE(
    alice/hollywood/library/scenarios/suggesters/movies/resources/suggestions.json suggestions.json
)

COPY_FILE(
    alice/hollywood/library/scenarios/suggesters/movies/resources/sample_suggestions.json ut/suggestions.json
)

END()
