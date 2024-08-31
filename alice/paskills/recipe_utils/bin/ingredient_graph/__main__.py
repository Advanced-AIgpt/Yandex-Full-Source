import argparse

from graphviz import Digraph

from alice.paskills.recipe_utils.lib.ingredients_group import read_ingredients_file


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--ingredients', required=True)
    args = parser.parse_args()

    ingredients = read_ingredients_file(args.ingredients)
    ingredient_map = {i.id: i for i in ingredients}
    dot = Digraph("Ingredient classes")

    for ingredient in ingredients:
        dot.node(ingredient.name)

    for parent in ingredients:
        for child_id in parent.children:
            child = ingredient_map[child_id]
            assert child is not None
            dot.edge(parent.name, child.name)

    dot.view()


if __name__ == '__main__':
    main()
