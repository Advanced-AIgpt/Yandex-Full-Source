# coding: utf-8
from __future__ import unicode_literals

import json
import os
import click
import logging
import re
import codecs
import pandas as pd

from tqdm import tqdm
from vins_core.utils.strings import smart_unicode
from vins_core.logger import set_default_logging
from vins_core.nlu.text_to_source import create_text_to_source_from_feature_cache


logger = logging.getLogger(__name__)

_UNDEFINED_SOURCE = 'Undefined source: this could happens in case of "data" source (e.g. microintens)'


def _load_text_to_source(filepath):
    logger.info(
        'Use existing text to source mappings %s (if feature cache has changed,'
        ' use "--force-reload" option to rebuild this mappings for safety)', filepath
    )
    with open(filepath) as f:
        return json.load(f)


def _iter_data(text, text_to_source):
    for i, info in enumerate(text_to_source.get(text, [])):
        yield (
            i,
            info['source'] or _UNDEFINED_SOURCE,
            info['text'],
            info['original_text']
        )


def _item_match_row_filters(item, row_filters):
    for column, pattern in row_filters.iteritems():
        if not isinstance(item[column], basestring):
            if pattern.startswith('>') and item[column] < float(pattern.lstrip('>')):
                return False
            if pattern.startswith('<') and item[column] > float(pattern.lstrip('<')):
                return False
        else:
            if not re.match(pattern, item[column]):
                return False
    return True


def _iter_plain_text(input_file):
    with codecs.open(input_file, encoding='utf-8') as f:
        for line in f:
            yield line.strip()


def _iter_tsv(input_file, text_col, row_filters=None):
    data = pd.read_csv(input_file, encoding='utf-8', sep=b'\t')
    for _, item in data.iterrows():
        if not row_filters or _item_match_row_filters(item, row_filters):
            yield item[text_col]


@click.command('do_main')
@click.argument('feature-cache', type=click.Path(exists=True))
@click.option('--force-reload', is_flag=True, help='Set this flag to rebuild precomputed *.text-to-source mappings')
@click.option('--input-file', help='If set, consume phrases from file rather than starting interactive prompt')
@click.option('--format', help='Input file format', type=click.Choice(['text', 'tsv']), default='text')
@click.option('--text-col', help='Text column name')
@click.option('--row-filters', help='Row filters', default='{}')
@click.option('--output-file', help='Output file (<input-file>.out by default)')
def do_main(feature_cache, force_reload, input_file, format, text_col, row_filters, output_file):
    set_default_logging('INFO')
    row_filters = json.loads(row_filters)
    text_to_source_file = feature_cache + '.text-to-source'
    if not os.path.exists(text_to_source_file) or force_reload:
        text_to_source = create_text_to_source_from_feature_cache(feature_cache, text_to_source_file)
    else:
        text_to_source = _load_text_to_source(text_to_source_file)

    if input_file:
        # get data from input file
        logger.info('Start processing input file {}'.format(input_file))
        _file_iter = None
        if format == 'text':
            _file_iter = _iter_plain_text(input_file)
        elif format == 'tsv':
            _file_iter = _iter_tsv(input_file, text_col, row_filters)

        output_file = output_file or os.path.join(input_file + '.out')
        with codecs.open(output_file, encoding='utf-8', mode='w') as fout:
            for text in tqdm(_file_iter):
                for _, path, _, original_text in _iter_data(text, text_to_source):
                    fout.write('{}\t{}\n'.format(original_text, path))

    else:
        # get data from interactive prompt
        logger.info('Prompt started, enter your phrase for seeking, for exit type "exit()"')
        while True:
            text = smart_unicode(raw_input('> '))
            if text == 'exit()':
                break
            for index, path, text, original_text in _iter_data(text, text_to_source):
                print('[======Source #{}=======]'.format(index))
                print('Path:\t{}'.format(path))
                print('Text:\t{}'.format(text))
                print('Original text:\t{}'.format(original_text))
                print('')


if __name__ == "__main__":
    do_main()
