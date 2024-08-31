#!/usr/bin/env python
# -*- coding: utf-8 -*-
import ujson
import click
import sys
from vins_core.config.app_config import load_app_config
from vins_core.dm.formats import FuzzyNLUFormat
from vins_core.utils.strings import smart_unicode, smart_utf8
from toloka_markup import TolokaMarkup, TolokaTag, Span


def read_stdin():
    for line in sys.stdin:
        line = line.strip()
        if not line or line.startswith('#'):
            continue
        yield smart_unicode(line)


def get_tags(item):
    tags = []
    for slot in item.slots:
        tags.append(TolokaTag(
            text=item.text[slot.begin:slot.end],
            label=slot.name,
            span=Span(pos=slot.begin, length=(slot.end - slot.begin))
        ))
    return tags


@click.command()
@click.option('-f', '--format', type=click.Choice(['tsv', 'json'], case_sensitive=False),
              default='tsv', help='Format of the output')
def do_main(format):
    app_cfg = load_app_config('personal_assistant')
    json_items = []
    if format == 'tsv':
        header = 'INPUT:query\tGOLDEN:output'
        click.echo(header, nl=True)
    for item in FuzzyNLUFormat.parse_iter(
        utterances=read_stdin(),
        templates=app_cfg.nlu_templates
    ).items:
        if not item.slots:
            continue
        if format == 'tsv':
            serialized_markup = TolokaMarkup(text=item.text, tags=get_tags(item)).serialize()
            click.echo(smart_utf8(serialized_markup), nl=True)
        elif format == 'json':
            json_items.append({
                'query': item.text,
                'output': TolokaMarkup(text='', tags=get_tags(item)).serialize_tags()
            })
    if json_items:
        click.echo(ujson.dumps(json_items, indent=2, ensure_ascii=False))


if __name__ == '__main__':
    do_main()
