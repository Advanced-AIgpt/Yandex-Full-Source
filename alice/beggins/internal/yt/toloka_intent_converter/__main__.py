# coding: utf-8

import collections.abc
import getpass
import re
import uuid

import click

import yt.wrapper

from .rules import RULES


class RuleRegistry:
    def __init__(self):
        self._rules = dict()

    def add_rule(self, pattern: str, intents: list[str]):
        matcher = re.compile(f'^(?:{pattern})$', flags=re.U | re.IGNORECASE)
        self._rules[matcher] = set(intents) | self._rules.get(matcher, set())

    def match(self, string: str) -> list[str]:
        matches = set()
        for matcher, intents in self._rules.items():
            if re.match(matcher, string):
                matches = matches | intents
        return list(matches)


class TolokaIntentConverter:
    def __init__(self, rule_registry: RuleRegistry, toloka_intent_column: str, converted_intent_column: str):
        self._rule_registry = rule_registry
        self._toloka_intent_column = toloka_intent_column
        self._converted_intent_column = converted_intent_column

    def __call__(self, row: dict):
        output_row = row
        value = row.get(self._toloka_intent_column, '')
        if isinstance(value, str):
            output = self._rule_registry.match(value)
        elif isinstance(value, collections.abc.Iterable):
            output = list({
                intent for toloka_intent in value for intent in self._rule_registry.match(toloka_intent)
            })
        else:
            raise TypeError(f'expected str or Iterable[str] but found {type(value)}')
        output_row[self._converted_intent_column] = output
        yield output_row


def get_tmp_table():
    return f'//tmp/{getpass.getuser()}/{uuid.uuid4().hex}'


def populate_rules(rule_registry: RuleRegistry):
    for pattern, intent in RULES.items():
        rule_registry.add_rule(pattern, [intent])


@click.command()
@click.option('--yt-proxy', default='hahn')
@click.option('--input-table', required=True)
@click.option('--output-table')
@click.option('--toloka-intent-column', default='toloka_intent')
@click.option('--converted-intent-column', default='intent')
def main(yt_proxy, input_table, output_table, toloka_intent_column, converted_intent_column):
    yt.wrapper.config.set_proxy(yt_proxy)
    output_table = output_table or get_tmp_table()
    registry = RuleRegistry()
    populate_rules(registry)
    mapper = TolokaIntentConverter(registry, toloka_intent_column, converted_intent_column)
    yt.wrapper.run_map(mapper, source_table=input_table, destination_table=output_table)
    print(f'Output table: https://yt.yandex-team.ru/{yt_proxy}/#page=navigation&offsetMode=row&path={output_table}')


if __name__ == '__main__':
    main()
