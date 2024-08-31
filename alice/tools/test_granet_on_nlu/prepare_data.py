#!/usr/bin/python

import yt.wrapper as yt
from yt.wrapper import read_table
from tqdm import tqdm
import json
import sys
import re
import click

yt.config['proxy']['url'] = 'hahn'
error = '.*\"Error\":.*'

@click.command()
@click.option('--intent', required=True, help='Intent name. Should be added to main.grnt')
@click.option('--table-path', default='//home/alice-dev/test_granet_on_nlu/nlu_requests', help='Path to table with requests.')
def main(intent, table_path):
    positive = open(intent + '.pos.tsv', 'w')
    negative = open(intent + '.neg.tsv', 'w')
    positive.write('text\tmock\t\n'), negative.write('text\tmock\tintent\t\n')

    for line in tqdm(read_table(table_path)):
        if re.match(error, line['mock']):
            continue
        if line['intent'] == intent:
            positive.write('{text}\t{mock}\t\n'.format(text=line['text'], mock=line['mock']))
        else:
            negative.write('{text}\t{mock}\t{intent}\t\n'.format(text=line['text'], mock=line['mock'], intent=line['intent']))


if __name__ == '__main__':
    main()

