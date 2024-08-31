#!/usr/bin/env python
# coding: utf-8

from __future__ import unicode_literals

import logging.config
import codecs
import click

from vins_core.dm.formats import FuzzyNLUFormat

click.disable_unicode_literals_warning = True

logger = logging.getLogger(__name__)


@click.command()
@click.argument('metric_file')
@click.argument('input_file')
@click.argument('output_file')
@click.option('--threshold', type=float, default=0.0)
def main(metric_file, input_file, output_file, threshold):
    """This script builds fst for custom ner type."""
    logging.config.dictConfig({
        'version': 1,
        'disable_existing_loggers': False,
        'formatters': {
            'standard': {
                'format': '[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s',
            },
            'colored': {
                '()': 'vins_core.logger.color_formatter.ColorFormatter',
                'format': '$COLOR[%(asctime)s] [%(request_id)s] [%(module)s:%(lineno)d] [%(levelname)s] %(message)s',
                'datefmt': '%Y-%m-%d_%H:%M:%S'
            }
        },
        'handlers': {
            'console': {
                'class': 'logging.StreamHandler',
                'level': 'DEBUG',
                'formatter': 'standard',
            }
        },
        'loggers': {
            '': {
                'handlers': ['console'],
                'level': 'DEBUG',
                'propagate': True,
            },
        },
    })

    influence_map = {}

    with codecs.open(metric_file, 'r', encoding='utf-8') as f:
        for s in f:
            splitted_string = s.strip().split('\t')
            utterance = splitted_string[0].strip()
            influence = splitted_string[2]
            influence_map[utterance] = float(influence)

    found = []

    with codecs.open(output_file, 'w', encoding='utf-8') as out:
        with codecs.open(input_file, 'r', encoding='utf-8') as inp:
            for s in inp:
                to_check = s.strip()

                if to_check and not to_check.startswith('#'):
                    to_check = FuzzyNLUFormat.parse_one(to_check).text
                    influence = influence_map.get(to_check)
                    if influence is not None and influence >= threshold:
                        found.append((influence, '%s: %s' % (influence, s)))
                        continue

                out.write(s)

        found = sorted(found, key=lambda x: -x[0])

        out.write('\n# Found by %s threshold\n' % threshold)

        for _, s in found:
            out.write(s)


if __name__ == '__main__':
    main()
