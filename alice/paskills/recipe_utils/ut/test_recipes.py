import os

import pytest

import yatest

from alice.paskills.recipe_utils.lib import granet


@pytest.fixture
def recipes_json():
    return os.path.join(
        yatest.common.source_path('alice/paskills/dialogovo/src/main/resources/recipes'),
        'recipes.json'
    )


@pytest.fixture
def recipes_granet():
    return os.path.join(
        yatest.common.source_path('alice/nlu/data/ru/granet/external_skills/recipes/entities'),
        'recipe.grnt'
    )


def test_all_json_recipes_are_in_granet(recipes_json, recipes_granet):
    missing_granet_recipes = granet.get_missing_granet_recipes(recipes_json, recipes_granet)
    assert missing_granet_recipes == set(), 'Following recipe ids are missing in granet file: ' + str(missing_granet_recipes)
