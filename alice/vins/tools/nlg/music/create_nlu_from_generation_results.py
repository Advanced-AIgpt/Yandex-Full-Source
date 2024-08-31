#!/usr/bin/env python
# coding: utf-8

from __future__ import unicode_literals

import logging.config
import codecs
import click
import json
import os
import random

from collections import defaultdict

from vins_core.ext.language_model import LanguageModelAPI


click.disable_unicode_literals_warning = True

logger = logging.getLogger(__name__)


def list_data_files(data_dir):
    for fname in os.listdir(data_dir):
        name = os.path.basename(fname)
        path = os.path.join(data_dir, name)

        if not os.path.isfile(path):
            continue

        name, ext = os.path.splitext(name)

        if ext != '.json' or 'stat' in name:
            continue

        yield name, path


@click.command()
@click.argument('input_directory')
@click.argument('output_path')
@click.option('--reduce_coeff', type=float, default=0.25)
@click.option('--normalize', is_flag=True, default=False)
@click.option('--mode', type=click.Choice(['3_gramm', '5_gramm']), default='3_gramm')
def main(input_directory, output_path, reduce_coeff, normalize, mode):
    FORMAT = '%(asctime)-15s %(message)s'
    logging.basicConfig(format=FORMAT, level=logging.DEBUG)

    language_model_api = LanguageModelAPI(
        mode=LanguageModelAPI.MODE_3_GRAMM if mode == '3_gramm' else LanguageModelAPI.MODE_5_GRAMM
    )

    all_phrases = defaultdict(list)

    for _, file in list_data_files(input_directory):
        with codecs.open(file, 'r', encoding='utf-8') as f:
            for phrase_info in json.load(f):
                form_string = json.dumps(phrase_info['form'], sort_keys=True, indent=2, ensure_ascii=False)
                ph = phrase_info['phrase']
                tagged_ph = phrase_info['tagged_phrase']
                score = language_model_api.get_score(ph)
                if normalize:
                    score /= len(ph.split())
                all_phrases[form_string].append((score, ph, tagged_ph,))

    phrases_by_type = defaultdict(list)

    for form_string, phrases in all_phrases.items():
        form = json.loads(form_string)
        phrases.sort(key=lambda x: -x[0])

        tp = form['type']
        if tp == 'switch_on':
            tp = tp + ('_discovery' if form['target']['type'] == 'collection' else '_ondemand')

        target_list = phrases_by_type[tp]

        for i in range(max(1, int(len(phrases) * reduce_coeff))):
            score, _, tagged_phrase = phrases[i]
            target_list.append('%s: %s\n' % (score, tagged_phrase))

    if not os.path.exists(output_path):
        os.makedirs(output_path)

    for tp, strings in phrases_by_type.items():
        output_file = os.path.join(output_path, tp + '.nlu')
        random.shuffle(strings)

        with codecs.open(output_file, 'w', encoding='utf-8') as f:
            for s in strings:
                f.write(s)


if __name__ == '__main__':
    main()
