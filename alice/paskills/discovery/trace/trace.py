import argparse
import json
from itertools import starmap

from .saas import ask_saas, get_skill_info
from .wizard import ask_wizard


def __make_table(table: [[str]], width: [int], header=None) -> str:
    def __merge(__row):
        def __shrink(__word, __size: int) -> str:
            __word = str(__word)[:__size]
            __word += (__size - len(__word)) * ' '
            return __word
        __cells = starmap(__shrink, zip(__row, width))
        return "| " + " | ".join(__cells) + " |"

    delim = "|{}|".format('-' * (sum(width) + 3 * len(width) - 1))
    rows = list(map(__merge, table))
    rows = [delim] + rows + [delim]
    if header:
        rows = [delim, __merge(header)] + rows
    return "\n".join(rows)


def __ask_query(query, skill_whitelist, **args):
    cm2 = ask_wizard(query, **args)
    skills, formula = ask_saas(query, **args)
    skills = list(map(lambda __s: (__s, skills.get(__s, 404)), skill_whitelist))
    for skill_id, relev in skills:
        __info = get_skill_info(skill_id)
        skill_name = __info.get('name', skill_id)
        isRec = 'OK' if __info.get('isRecommended', False) else 'notR'
        yield (query, skill_name, cm2,  relev, isRec, formula)


def ask_patrons(queries, skill_whitelist, **args):
    __TABLE_WIDTH = [35, 16, 4, 4, 10]
    __HEADER = ['Query', 'Skill', 'CM2', 'SaaS', 'Moderation']
    table = [__row for __query in queries for __row in __ask_query(__query, skill_whitelist, **args)]
    table = __make_table(table, __TABLE_WIDTH, __HEADER)
    print(table)


def read_patrons(filename: str, **args) -> ([str], [str]):
    with open(filename) as f:
        patrons = json.load(f)
        queries = patrons.get('query', [])
        whitelist = patrons.get('skill_whitelist', [])
    if 'query_text' in args and args['query_text']:
        queries = [args['query_text']]
    if 'skill_id' in args and args['skill_id']:
        whitelist = [args['skill_id']]
    return (queries, whitelist)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('patrons')
    parser.add_argument('--saas-output', type=str)
    parser.add_argument('--wizard-output', type=str)
    parser.add_argument('--saas-formula', type=str)
    parser.add_argument('--saas-kps', type=int)
    parser.add_argument('--saas-threshold', type=int)
    parser.add_argument('--saas-softness', type=int)
    parser.add_argument('--query-text', type=str)
    parser.add_argument('--skill-id', type=str)
    args = parser.parse_args()

    queries, skill_whitelist = read_patrons(args.patrons, **vars(args))
    print('Queries: {}\nSkill whitelist: {}'.format(queries, skill_whitelist))
    ask_patrons(queries, skill_whitelist, **vars(args))


if __name__ == "__main__":
    main()
