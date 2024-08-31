import difflib
import json
from queue import Queue

from typing import List


DEFINETELY_NOT_CONNECTED = set([
    ('apple', 'milk'),
    ('apple', 'milk_powder'),
    ('apple', 'baked_milk'),
    ('celery_leaves', 'green_onion'),
    ('cream', 'olives'),
    ('cumin', 'rice_dev_zira'),
    ('egg_protein', 'egg_yolk'),
    ('pickled_cucumbers', 'pickled_mushrooms'),
    ('pickled_cucumber', 'pickled_mushrooms'),
    ('soda', 'sparkling_water'),
    ('soda', 'water'),
    ('soda', 'water_boiled'),
    ('beef_bone', 'mutton_with_bone'),
    ('chicken', 'cinnamon'),
    ('cucumber', 'greek_yogurt'),
    ('baked_milk', 'melted_butter'),
    ('yogurt', 'cucumber'),
    ('yogurt', 'pickled_cucumbers'),
    ('greek_yogurt', 'pickled_cucumbers'),
    ('yogurt_natural', 'pickled_cucumbers'),
    ('yogurt', 'pickled_cucumber'),
    ('greek_yogurt', 'pickled_cucumber'),
    ('yogurt_natural', 'pickled_cucumber'),
    ('greek_yogurt', 'cucumber_uncountable'),
    ('yogurt_natural', 'cucumber_uncountable'),
    ('yogurt', 'cucumber_uncountable'),
    ('cucumber', 'yogurt_natural'),
    ('cucumber', 'greek_yogurt'),
    ('canned_pink_salmon', 'tomatoes_in_their_own_juice'),
    ('caviar_red', 'red_onion'),
    ('celery_leaves', 'greens'),
    ('green_onion', 'greens'),
    ('mutton_with_bone', 'beef_with_bone'),
    ('vanilla_ground', 'walnut_ground'),
    ('vanilla_ground', 'paprika_ground'),
    ('paprika_ground', 'hot_pepper_ground'),
    ('hot_pepper_ground', 'vanilla_ground'),
    ('hot_pepper_ground', 'walnut_ground'),
    ('hot_pepper_ground', 'paprika_ground'),
    ('paprika_ground', 'walnut_ground'),
    ('cream', 'plum'),
    ('green_onion', 'pickled_cucumber'),
    ('black_pepper_ground', 'paprika_ground'),
    ('lemon_peel', 'lemon_juice'),
    ('lemon_peel', 'lemon'),
    ('canned_pink_salmon', 'lemon_juice'),
    ('tomatoes_in_their_own_juice', 'lemon_juice'),
    ('lemon', 'lemon_juice'),
    ('black_pepper_ground', 'vanilla_ground'),
    ('black_pepper_ground', 'walnut_ground'),
    ('wheat_bread', 'wheat_flour'),
    ('apple', 'coconut_milk'),
    ('coconut_oil', 'coconut_milk'),
    ('chicken_fillet', 'minced_chicken'),
    ('tomato_paste', 'curry_paste'),
    ('apple_uncountable', 'milk'),
    ('apple_uncountable', 'milk_powder'),
    ('apple_uncountable', 'baked_milk'),
    ('apple_uncountable', 'coconut_milk'),
])


class Ingredient:

    def __init__(self, json_ingredient):
        self.id = json_ingredient['id']
        self.name = json_ingredient['name']
        self.children = json_ingredient.get('children', [])
        self.parents = set()

    @property
    def name_words(self):
        return [word for word in self.name.split(' ') if word.strip()]

    def __str__(self):
        return f'Ingredient(id={self.id}, name={self.name})'

    def __repr__(self):
        return str(self)

    def __eq__(self, other):
        return isinstance(other, Ingredient) and self.id == other.id

    def __hash__(self):
        return hash(self.id)


def words_are_similar(a: str, b: str, threshold: float = 0.6):
    shortest_word_len = max(len(a), len(b))
    seq_match = difflib.SequenceMatcher(None, a, b, False)
    longest_match = seq_match.find_longest_match(0, len(a), 0, len(b))
    longest_match_len = longest_match.size
    distance = (float(longest_match_len) / shortest_word_len)
    return distance > threshold


def ingredients_are_connected(first, second):
    """
    ingredients are connected if they have at least one common parent
    """
    return (
        first in second.parents
        or second in first.parents
        or len(first.parents.intersection(second.parents)) > 0
    )


def compare_names(first, second):
    if first == second or (first.id, second.id) in DEFINETELY_NOT_CONNECTED or (second.id, first.id) in DEFINETELY_NOT_CONNECTED:
        return None
    for first_word in first.name_words:
        similar_words = set()
        for second_word in second.name_words:
            if words_are_similar(first_word, second_word) and not ingredients_are_connected(first, second):
                similar_words.add((first_word, second_word))
        if similar_words:
            return similar_words
    return None


def fill_parents(ingredients: List[Ingredient]):
    def walk_and_fill_parents(parent):
        nodes_to_visit = Queue()
        nodes_to_visit.put(parent)
        visited_nodes = set()
        while not nodes_to_visit.empty():
            current_node = nodes_to_visit.get()
            if current_node.id in ('nuts', 'walnut'):
                print(current_node.children)
            visited_nodes.add(current_node)
            for child_id in current_node.children:
                child = ingredient_map[child_id]
                child.parents.add(current_node)
                child.parents.update(p for p in current_node.parents)
                if child not in visited_nodes:
                    nodes_to_visit.put(child)

    ingredient_map = {i.id: i for i in ingredients}
    for parent in ingredients:
        walk_and_fill_parents(parent)


def read_ingredients_file(ingredients_file: str) -> List[Ingredient]:
    with open(ingredients_file) as f:
        ingredients = [Ingredient(json_ingredient) for json_ingredient in json.loads(f.read())]
    fill_parents(ingredients)
    return ingredients


def group_ingredients(ingredients_file: str):
    ingredients = read_ingredients_file(ingredients_file)

    similar_ingredients = []
    visited_pairs = set()
    for first in ingredients:
        for second in ingredients:
            if (first, second) not in visited_pairs:
                similar_words = compare_names(first, second)
                if similar_words:
                    print(f'{first} and {second} have similar word pairs: {similar_words} but no connection')
                    similar_ingredients.append((first, second))
                visited_pairs.add((first, second))
                visited_pairs.add((second, first))
    return similar_ingredients
