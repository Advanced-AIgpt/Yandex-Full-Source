#!/usr/bin/env python
# -*- coding: utf-8 -*-
import click
import itertools
import os.path
import yt.wrapper as yt


TOLOKA_TAGGING_PROJECT_BASE_PATH = '//home/voice/dialog/toloka/tagger'
yt.config['proxy']['url'] = os.environ.get('YT_PROXY', 'hahn')


def get_tables(project_path, date_from, date_to):
    tables = []
    for date in yt.list(project_path):
        if (date_from is None or date >= date_from) and (date_to is None or date <= date_to):
            tables.append(os.path.join(project_path, date))
    return tables


def read_rows(project_path, date_from, date_to):
    for table in get_tables(project_path, date_from, date_to):
        for row in yt.read_table(table, format='<encode_utf8=%false>json'):
            yield row


def aggregate_answers(row):
    result = []
    intent_confidence = 1.0 if row['intent_confidence'] is None else row['intent_confidence']
    for output, same_outputs in itertools.groupby(sorted(row['outputs'])):
        count = len(list(same_outputs))
        share = float(count) / len(row['outputs'])
        result.append((row['query'], output, share, row['toloka_intent'], intent_confidence, row['app']))
    return result


@click.command()
@click.option('-p', '--project-name', prompt='Project name (e.g. show_route)',
              help='Toloka tagging project name (e.g. show_route)')
@click.option('-f', '--date-from', default=None,
              help='Date (YYYY-MM-DD) to start fetching from. Earliest possible date is chosen by default')
@click.option('-t', '--date-to', default=None,
              help='Date (YYYY-MM-DD) to end fetching on. Most recent available date is chosen by default')
def do_main(project_name, date_from, date_to):
    project_path = os.path.join(TOLOKA_TAGGING_PROJECT_BASE_PATH, project_name)
    for row in read_rows(project_path, date_from, date_to):
        for aggregated in aggregate_answers(row):
            click.echo(u'\t'.join(map(unicode, aggregated)))


if __name__ == '__main__':
    do_main()
