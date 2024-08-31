import os

import pytest

import yatest

from alice.paskills.recipe_utils.lib import ingredients_group, granet


@pytest.fixture
def ingredients_json():
    return os.path.join(
        yatest.common.source_path('alice/paskills/dialogovo/src/main/resources/recipes'),
        'ingredients.json'
    )


@pytest.fixture
def ingredients_granet():
    return os.path.join(
        yatest.common.source_path('alice/nlu/data/ru/granet/external_skills/recipes/entities'),
        'ingredient.grnt'
    )


def test_find_similar_ingredients_without_connections(ingredients_json):
    probably_similar_ingredients = ingredients_group.group_ingredients(ingredients_json)
    assert probably_similar_ingredients == [], f'Found probably {len(probably_similar_ingredients)} similar ingredient pairs'


def test_all_json_ingredients_are_in_granet(ingredients_json, ingredients_granet):
    missing_granet_ingredients = granet.get_missing_granet_ingredients(ingredients_json, ingredients_granet)
    assert missing_granet_ingredients == set(), 'Following ingredients are missing in granet file: ' + str(missing_granet_ingredients)
