import argparse

from alice.paskills.recipe_utils.lib import ingredients_group


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--ingredients', required=True)
    args = parser.parse_args()
    ingredients_group.group_ingredients(args.ingredients)


if __name__ == '__main__':
    main()
