#!/usr/bin/env python
# coding: utf-8
from __future__ import unicode_literals

import argparse
import logging
import logging.config

import codecs

from vins_core.ext.voice_synth import VoiceSynthAPI

logger = logging.getLogger(__name__)


def execute_tts_on_dataset(input_file, output_prefix, speaker, emotion):
    logger.info('Processing %s', input_file)

    voice_synth = VoiceSynthAPI(timeout=10)

    with codecs.open(input_file, 'r', encoding='utf-8') as f_in:
        lineno = 0
        for line in f_in:
            line = line.strip()

            if not line:
                continue

            logger.info('Sending %s to %s', line, voice_synth.url)

            try:
                response = voice_synth.submit(line, emotion=emotion, speaker=speaker)
                out_path = output_prefix + '_' + str(lineno) + '.mp3'
                logger.info('Writing result to %s', out_path)

                with codecs.open(out_path, 'wb') as out:
                    out.write(response.content)
            except Exception as e:
                logger.error('Error: %s! Skipping.', e.message, line)

            lineno += 1


def main():
    parser = argparse.ArgumentParser(add_help=True, description='Transform each string in the input files into '
                                                                'short mp3 file using speechkit tts API. '
                                                                'Generated files will be named '
                                                                '<output_prefix>_<lineno>.mp3 where output_prefix '
                                                                'is specified by user and lineno is the '
                                                                'current string number in the input file')
    parser.add_argument(
        '--input-file', metavar='FILE', required=True, help='input dataset'
    )
    parser.add_argument(
        '--output-prefix', type=str, required=True, help='output files prefix')
    parser.add_argument(
        '--speaker', choices=['jane', 'oksana', 'alyss', 'omazh', 'zahar', 'ermil'],
        required=False, default='oksana', help='speaker voice')
    parser.add_argument(
        '--emotion', choices=['good', 'neutral', 'evil'],
        required=False, default='good', help='speaker voice')

    args = parser.parse_args()

    logging.config.dictConfig({
        'version': 1,
        'disable_existing_loggers': False,
        'formatters': {
            'standard': {
                'format': '[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s',
            },
        },
        'handlers': {
            'console': {
                'class': 'logging.StreamHandler',
                'level': 'INFO',
                'formatter': 'standard',
            },
        },
        'loggers': {
            '': {
                'handlers': ['console'],
                'level': 'INFO',
                'propagate': True,
            },
        },
    })

    execute_tts_on_dataset(
        input_file=args.input_file,
        output_prefix=args.output_prefix,
        speaker=args.speaker,
        emotion=args.emotion
    )


if __name__ == "__main__":
    main()
