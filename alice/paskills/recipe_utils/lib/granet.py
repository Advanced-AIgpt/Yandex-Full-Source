import re
import json

from alice.paskills.recipe_utils.lib.ingredients_group import read_ingredients_file

# TODO (ivangromov): use granet bindings instead of parse_granet_entities()

ENTITY_NAME_RE = re.compile(r"^entity\s+([\w\.]+):\s*$")
VALUES_START_RE = re.compile(r"    values:\s*")
VALUE_RE = re.compile("        (\w+)\s*")


# list of ingredient ids that should not be listed in granet entity
IGNORE_INGREDIENTS = {
    'avocado_uncountable',
    'beetroot_uncountable',
    'carrot_uncountable',
    'cucumber_uncountable',
    'onion_uncountable',
    'pumpkin_uncountable',
    'squash_uncountable',
    'sweet_pepper_uncountable',
    'tomato_uncountable',
    'none',
    'potato_uncountable',
    'loaf_uncountable',
    'bay_leaf_uncountable',
    'pickled_cucumbers',
    'black_pepper_ground',
    'lemon_juice',
    'lemon_peel',
    'apple_uncountable',
}


class Entity:
    def __init__(self, name, values):
        self.name = name
        self.values = values


def read_granet_entities(granet_filename):
    current_entity_name = None
    current_entity_values = []
    parsing_values = False
    entities = []
    with open(granet_filename) as f:
        for line in f:
            entity_name_match = ENTITY_NAME_RE.match(line)
            value_match = VALUE_RE.match(line)
            if entity_name_match is not None:
                current_entity_name = entity_name_match.groups()[0]
            elif VALUES_START_RE.match(line):
                parsing_values = True
            elif parsing_values and value_match:
                current_entity_values.append(value_match.groups()[0])
            elif line.strip() == '':
                if current_entity_name is not None and current_entity_values:
                    entities.append(Entity(current_entity_name, current_entity_values))
                current_entity_name = None
                current_entity_values = []
                parsing_values = False
    if current_entity_name is not None and current_entity_values:
        entities.append(Entity(current_entity_name, current_entity_values))
    return {e.name: e for e in entities}


def get_missing_granet_ingredients(ingredients_filename, granet_filename):
    ingredient_entity = read_granet_entities(granet_filename).get('Ingredient')
    if ingredient_entity is None:
        raise ValueError(f"Couldn't parse entity from granet file {granet_filename}")
    granet_ingredient_ids = set(ingredient_entity.values)
    json_ingredient_ids = set([i.id for i in read_ingredients_file(ingredients_filename)])
    return json_ingredient_ids - IGNORE_INGREDIENTS - granet_ingredient_ids


def get_missing_granet_recipes(recipes_filename, granet_filename):
    recipe_entity = read_granet_entities(granet_filename).get('Recipe')
    if recipe_entity is None:
        raise ValueError(f"Couldn't parse entity from granet file {granet_filename}")
    granet_recipe_ids = set(recipe_entity.values)
    with open(recipes_filename) as recipes_file:
        json_recipe_ids = set([r['id'] for r in json.loads(recipes_file.read())])
    return json_recipe_ids - granet_recipe_ids
