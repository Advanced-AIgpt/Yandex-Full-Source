# coding: utf-8

import argparse
import json
import logging
import os.path
import yt.wrapper as yt

from tqdm import tqdm

from alice.nlu.py_libs.request_normalizer import request_normalizer, lang
from alice.nlu.py_libs.utils import fuzzy_nlu_format, sample as sample_m, sample_normalizer
from alice.nlu.py_libs.utils.strings import smart_unicode


logger = logging.getLogger(__name__)


class FstNormalizerRuWrapper(object):
    def __init__(self):
        self._fst_normalizer = request_normalizer.RequestNormalizer()
        self._cache = {}

    def __call__(self, text):
        if text in self._cache:
            return self._cache[text]
        result = self._fst_normalizer.normalize(lang.Lang.RUS, smart_unicode(text))
        self._cache[text] = result
        return result


_VINSFILE_PATH = 'personal_assistant/config/Vinsfile.json'

_DEFAULT_NLU_TEMPLATES = {
    'address_ru': 'address_ru.txt',
    'city_ru': 'city_ru.txt',
    'country': 'country.txt',
    'region_ru': 'region_ru.txt',
    'poi_ru': 'poi_ru.txt',
    'poi_activity_ru': 'poi_activity_ru.txt',
    'yandex_search_query': 'yandex_search_query.txt',
    'first_name_ru': 'first_name_ru.txt',
    'middle_name_ru': 'middle_name_ru.txt',
    'last_name_ru': 'last_name_ru.txt',
    'date_ru': 'date_ru.txt',
    'day_of_week_ru': 'day_of_week_ru.txt',
    'abs_time_ru': 'abs_time_ru.txt',
    'currency': 'currency.txt',
    'rel_time_ru': 'rel_time_ru.txt',
}


def load_nlu_template(path):
    data = []
    with open(path) as f:
        for line in f:
            line = line.strip()
            if line and not line.startswith('#'):
                data.append(smart_unicode(line))
    return fuzzy_nlu_format.FuzzyNLUTemplate(data=data)


def load_custom_entity_nlu_template(path):
    with open(path) as f:
        entity = json.load(f)
    data = []
    for key, synonyms in entity.items():
        for synonym in synonyms:
            data.append(smart_unicode(synonym))
    return fuzzy_nlu_format.FuzzyNLUTemplate(data=data)


def load_custom_entity_nlu_templates(vins_source_path, vinsprojectfile_path):
    nlu_templates = {}
    with open(os.path.join(vins_source_path, vinsprojectfile_path)) as f:
        vinsproject = json.load(f)
    for entity_info in vinsproject.get('entities', []):
        nlu_templates['ce_' + entity_info['entity']] = load_custom_entity_nlu_template(os.path.join(vins_source_path, entity_info['path']))
    return nlu_templates


def load_nlu_templates(default_nlu_template_path, vins_source_path):
    nlu_templates = {}
    for name, filename in _DEFAULT_NLU_TEMPLATES.items():
        nlu_templates[name] = load_nlu_template(os.path.join(default_nlu_template_path, filename))
    with open(os.path.join(vins_source_path, _VINSFILE_PATH)) as f:
        vinsfile = json.load(f)
    for name, path in vinsfile['nlu']['custom_templates'].items():
        nlu_templates[name] = load_nlu_template(os.path.join(vins_source_path, path))
    for project_info in vinsfile['project']['includes']:
        nlu_templates.update(load_custom_entity_nlu_templates(vins_source_path, project_info['path']))
    return nlu_templates


def load_intent_nlu(path, name, can_use_to_train_tagger, unique, nlu_templates):
    with open(path) as f:
        logger.info('Processing file %s', path)
        texts = f.readlines()
        if unique:
            texts = list(set(texts))
        return fuzzy_nlu_format.FuzzyNLUFormat.parse_iter(
            utterances=texts,
            name=name,
            templates=nlu_templates,
            can_use_to_train_tagger=can_use_to_train_tagger
        )


def load_project_nlu(vins_source_path, vinsprojectfile_path, base_name, nlu_templates):
    with open(os.path.join(vins_source_path, vinsprojectfile_path)) as f:
        vinsproject = json.load(f)
    for intent_info in vinsproject.get('intents', []):
        intent_name = '{0}.{1}.{2}'.format(base_name, vinsproject['name'], intent_info['intent'])
        for nlu_info in intent_info.get('nlu', []):
            if nlu_info.get('source') != 'file':
                logger.warning('Skipping "%s" source for intent %s. Only "file" sources supported.',
                               nlu_info.get('source'), intent_info.get('name'))
                continue
            can_use_to_train_tagger = nlu_info.get('can_use_to_train_tagger', True)
            if not can_use_to_train_tagger:
                continue
            yield intent_name, load_intent_nlu(
                os.path.join(vins_source_path, nlu_info['path']),
                intent_info.get('name'),
                can_use_to_train_tagger,
                nlu_info.get('unique', True),
                nlu_templates
            )


def load_nlu_data(nlu_templates, vins_source_path):
    with open(os.path.join(vins_source_path, _VINSFILE_PATH)) as f:
        vinsfile = json.load(f)
    for project_info in vinsfile['project']['includes']:
        for intent_name, data in load_project_nlu(vins_source_path, project_info['path'], vinsfile['project']['name'], nlu_templates):
            yield intent_name, data


def _collect_data(default_nlu_template_path, vins_source_path):
    nlu_templates = load_nlu_templates(default_nlu_template_path, vins_source_path)
    normalizer = sample_normalizer.SampleNormalizer(FstNormalizerRuWrapper())
    for intent_name, data in tqdm(load_nlu_data(nlu_templates, vins_source_path)):
        if not any([item.slots for item in data.items]):
            continue
        for item in data.items:
            sample = sample_m.Sample.from_nlu_source_item(item)
            try:
                sample = normalizer.normalize(sample)
            except sample_normalizer.FstNormalizerError as e:
                logger.warn(e.args[0] % e.args[1:])
                continue
            yield {
                'intent': intent_name,
                'utterance_text': sample.text,
                'markup': sample.to_markup(),
                'extra': item.extra,
                'can_use_to_train_tagger': item.can_use_to_train_tagger
            }


def do_main():
    logging.basicConfig(level=logging.INFO, format='[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s')

    parser = argparse.ArgumentParser()
    parser.add_argument('--table-path', required=True)
    parser.add_argument('--vins-source-path', required=True)
    parser.add_argument('--default-nlu-template-path', required=True)
    args = parser.parse_args()

    yt.remove(args.table_path, force=True)
    schema = [
        {"name": "extra", "type": "string"},
        {"name": "can_use_to_train_tagger", "type": "boolean"},
        {"name": "intent", "type": "string"},
        {"name": "markup", "type": "string"},
        {"name": "utterance_text", "type": "string"}
    ]
    yt.create("table", args.table_path, attributes={"schema": schema})
    yt.write_table(args.table_path, _collect_data(args.default_nlu_template_path, args.vins_source_path))


def main():
    do_main()


if __name__ == "__main__":
    do_main()
