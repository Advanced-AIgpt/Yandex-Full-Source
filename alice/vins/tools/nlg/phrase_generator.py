# coding: utf-8
from __future__ import unicode_literals

import logging.config
import json
import os
import codecs
import argparse
import random
import numpy
import datetime

from vins_core.nlg.template_nlg import Template, get_env
from personal_assistant.ontology.onto import Ontology
from personal_assistant.ontology.onto_synthesis_engine import OntologicalSynthesisEngine

logger = logging.getLogger(__name__)

SAMPLE_FROM_ENTITY = 'sample_from_entity'
SAMPLE_FROM_LIST = 'sample_from_list'
SAMPLE_FROM_FILE = 'sample_from_file'
SAMPLE_FROM_JSON_LIST = 'sample_from_json_list'
SAMPLE_FROM_DATETIME_RAW = 'sample_from_datetime_raw'


POSSIBLE_TASKS = [
    SAMPLE_FROM_ENTITY,
    SAMPLE_FROM_LIST,
    SAMPLE_FROM_FILE,
    SAMPLE_FROM_JSON_LIST,
    SAMPLE_FROM_DATETIME_RAW
]


def random_datetime_raw(only_date):
    result = {}
    components = ['years', 'months', 'weeks', 'days'] if only_date else [
        'years', 'months', 'weeks', 'days', 'hours', 'minutes', 'seconds'
    ]

    relative = random.choice([-1, 1])
    relative_lower_bound = 0
    zero_relative_generated = False

    max_number_of_components = 1 if random.random() < 0.5 else len(components)

    for i in range(len(components)):
        if not max_number_of_components:
            break
        cur_component = components[i]

        if random.random() < 0.25 and max_number_of_components > 1:
            relative = 0

        if random.random() < 0.5 or cur_component == 'weeks' and not relative:
            continue

        val = None

        if relative:
            val = random.randint(relative_lower_bound, 2)
        else:
            if cur_component == 'years':
                val = random.randint(2017, 2018)
            elif cur_component == 'months':
                val = random.randint(1, 2)
            elif cur_component == 'days':
                val = random.randint(1, 2)
            elif cur_component == 'hours':
                val = random.randint(0, 2)
            elif cur_component == 'minutes':
                val = random.randint(0, 2)
            elif cur_component == 'seconds':
                val = random.randint(0, 2)

        # The only chance to generate zero relative is to generate it at the first iteration
        relative_lower_bound = 1

        result[cur_component] = val * relative if relative else val
        max_number_of_components -= 1

        if relative:
            result['%s_relative' % cur_component] = True

        if relative and not val:
            # do not try to extend zero relative with other information
            zero_relative_generated = True
            break

    if not zero_relative_generated and (random.random() < 0.25 if len(result) > 2 else 0.5):
        result['weekday'] = random.randint(1, 7)

    if not result:
        result = {'days_relative': True, 'days': 0}

    return result


def sample_from_entity(params):
    if params.get('optional') == 'true' and random.random() < 0.5:
        return None
    instances = list(ontology[params['entity_name']].instances)

    if 'weights' not in params:
        return random.choice(instances).name

    sum_weights = 0.0
    for i in instances:
        sum_weights += params['weights'].get(i.name, 1.0)

    probs = [params['weights'].get(i.name, 1.0) / sum_weights for i in instances]
    return numpy.random.choice(instances, 1, p=probs)[0]


parsed_text_lists = {}


def sample_from_file(params):
    big_list = []
    for p in params['path']:
        path = os.path.join(os.path.dirname(args.config_path), p)
        if path not in parsed_text_lists:
            lst = []
            with codecs.open(path, 'r', encoding='utf-8') as f_in:
                if params.get('custom_entity'):
                    lst = sum(json.load(f_in).itervalues(), [])
                else:
                    for line in f_in:
                        line = line.strip()
                        if line:
                            lst.append(line)
            parsed_text_lists[path] = lst

        big_list += parsed_text_lists[path]
    return random.choice(big_list)


parsed_json_lists = {}


def sample_from_json(params):
    big_list = []
    for p in params['path']:
        path = os.path.join(os.path.dirname(args.config_path), p)

        if path not in parsed_json_lists:
            lst = []
            with codecs.open(path, 'r', encoding='utf-8') as f_in:
                list_of_slot_values = json.load(f_in)
                for slot_value in list_of_slot_values:
                    value = slot_value.get('value')
                    if value:
                        lst.append(value)
            parsed_json_lists[path] = lst

        big_list += parsed_json_lists[path]
    return random.choice(big_list)


def choose_value(task, params):
    if task == SAMPLE_FROM_LIST:
        return random.choice(params)
    elif task == SAMPLE_FROM_ENTITY:
        return sample_from_entity(params)
    elif task == SAMPLE_FROM_FILE:
        return sample_from_file(params)
    elif task == SAMPLE_FROM_JSON_LIST:
        return sample_from_json(params)
    elif task == SAMPLE_FROM_DATETIME_RAW:
        r = random_datetime_raw(params.get('only_date', False))
        return r


def prepare_form(form):
    result = {}
    for k, v in form.items():
        if isinstance(v, list) and v and v[0] in POSSIBLE_TASKS:
            v = choose_value(v[0], v[1])
        if v:
            result[k] = v
    return result


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Generate phrases using VINS nlg subsystem.\n'
                                                 'Usage: python phrase_generator.py '
                                                 '--nlg-path=<path to jinjafile.nlg> '
                                                 '--config-path=<path to generator json config>'
                                                 '--out-path=<path to file with generated phrases>',
                                     formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument('--nlg-path', type=str, required=True, help='path to jinjafile.nlg')
    parser.add_argument('--config-path', type=str, required=True, help='path to generator json config')
    parser.add_argument('--out-path', type=str, required=True, help='path where to store generated phrases')
    parser.add_argument('--shortest-top-size', type=int, default=100,
                        required=False, help='defines the size of shortest phrases toplist')
    parser.add_argument('--top-path', type=str, required=False, help='path where to store shortest toplist json')

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
                'level': 'WARNING',
                'formatter': 'standard',
            },
        },
        'loggers': {
            '': {
                'handlers': ['console'],
                'level': 'WARNING',
                'propagate': True,
            },
        },
    })

    config_file = open(args.config_path)
    config = json.load(config_file)

    if 'ontology_path' in config:
        ontology = Ontology.from_file(os.path.join(os.path.dirname(args.config_path), config['ontology_path']))
        synth_engine = OntologicalSynthesisEngine(ontology)
    else:
        ontology = synth_engine = None

    includes = [os.path.join(os.path.dirname(args.config_path), config.get('nlg_include_path', ''))]

    with codecs.open(args.nlg_path, encoding='utf-8') as f:
        nlg_data = f.read()

    template = Template(nlg_data, env=get_env(includes))
    shortest_toplist = []

    def context_generator():
        for f in config['form']:
            n = f['number_of_iterations'] if 'number_of_iterations' in f else config['number_of_iterations']
            for _ in range(n):
                for phrase_id in config['phrase_ids']:
                    yield (f, phrase_id)

    unique_phrases = set()

    n_tries = 0
    for source_form, phrase_id in context_generator():
        while True:
            prepared_form = prepare_form(source_form)
            phrase = template.render_phrase(
                form=prepared_form,
                phrase_id=phrase_id,
                context={'onto': ontology, 'onto_synth': synth_engine}
            ).text
            if phrase in unique_phrases:
                if n_tries > 100:
                    break
                n_tries += 1
                continue
            n_tries = 0
            unique_phrases.add(phrase)
            shortest_toplist.append((prepared_form, phrase,))

            break

    shortest_toplist.sort(key=lambda (form, phrase): len(phrase))

    with codecs.open(args.out_path, 'w', encoding='utf-8') as f_out:
        timestamp = datetime.datetime.now().strftime("%d.%m.%Y, %H:%M:%S")
        f_out.write(
            '# This file was autogenerated by phrase_generator.py\n'
            '# using grammar from %s on %s\n\n' % (args.nlg_path, timestamp)
        )
        for _, phrase in shortest_toplist:
            f_out.write(phrase)
            f_out.write('\n')

    if args.top_path:
        shortest_toplist = shortest_toplist[0:args.shortest_top_size]

        toplist_json = json.dumps(
            shortest_toplist, sort_keys=False, ensure_ascii=False, indent=2, separators=(',', ': ')
        )
        with codecs.open(args.top_path, 'w', encoding='utf8') as f:
            f.write(toplist_json + '\n')
