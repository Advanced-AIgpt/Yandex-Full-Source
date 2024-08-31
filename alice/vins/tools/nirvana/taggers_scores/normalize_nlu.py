# coding: utf-8

import logging
import click
import codecs
import importlib
import json

from vins_core.dm.formats import FuzzyNLUFormat
from vins_core.common.sample import Sample
from vins_core.utils.data import find_vinsfile
from vins_core.common.slots_map_utils import tags_to_slots

logger = logging.getLogger(__name__)


_aliases = {
    'personal_assistant': 'personal_assistant.app::PersonalAssistantApp',
    'navi_app': 'navi_app.app::NaviApp',
    'gc_skill': 'gc_skill.app::ExternalSkillApp'
}


def create_app(app_name):
    app_path = _aliases.get(app_name)
    module_name, app_class_name = app_path.split('::')
    app_class = getattr(importlib.import_module(module_name), app_class_name)

    app = app_class(
        vins_file=find_vinsfile(module_name),
        load_data=False,
    )

    return app


def _iterate_nlu(input_file):
    with codecs.open(input_file, encoding='utf-8') as f:
        for item in FuzzyNLUFormat.parse_iter(f).items:
            sample = Sample.from_nlu_source_item(item)
            yield sample.text, sample


def _drop_entities(slots):
    def delete_key(d, key):
        return {i: d[i] for i in d if i != key}

    return {slot: [delete_key(val, 'entities') for val in values] for slot, values in slots.iteritems()}


@click.command()
@click.argument('input_file', type=click.Path(exists=True))
@click.argument('output_file', type=click.Path())
@click.option('--app_name', default='personal_assistant', type=click.Choice(_aliases.keys()))
def main(input_file, output_file, app_name):
    app = create_app(app_name)

    with codecs.open(output_file, 'wt', encoding='utf-8') as fd:
        for sample_text, sample in _iterate_nlu(input_file):
            sample = app.samples_extractor([sample], session=None, filter_errors=True)[0]
            slots, _ = tags_to_slots(sample.tokens, sample.tags)
            slots = _drop_entities(slots)
            item = {
                'text': sample_text,
                'normalized_text': sample.text,
                'slots': slots
            }
            fd.write('{}\n'.format(json.dumps(item)))


if __name__ == '__main__':
    main()
