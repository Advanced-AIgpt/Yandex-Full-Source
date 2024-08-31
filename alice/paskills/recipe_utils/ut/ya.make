OWNER(g:paskills)

PY3TEST()

TEST_SRCS(
    test_ingredients.py
    test_recipes.py
)

PEERDIR(
    alice/paskills/recipe_utils/lib
)

DATA(
    arcadia/alice/paskills/dialogovo/src/main/resources/recipes
    arcadia/alice/nlu/data/ru/granet/external_skills/recipes/entities
)

END()
