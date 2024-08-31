# -*- coding: utf-8 -*-

from dataclasses import dataclass
import click
import itertools
import os
import os.path
import re
import sys
import yaml


YA_MAKE_TEMPLATE = '''
# WARNING! Do not edit manually. Use alice/vins/tools/tagger/update_resources to update this file

OWNER(
    g:alice_quality
    g:begemot
)

UNION()

{sandbox_resources}

END()
'''


RESOURCE_TEMPLATE = '''
FROM_SANDBOX(
    {resource_id}
    PREFIX
    personal_assistant_model_directory
{model_file_paths}
)
'''.rstrip()


MODEL_FILES = [
    'model.mmap',
    'model_description'
]


MODEL_PATH_PATTERN = re.compile(
    r'personal_assistant_model_directory/tagger/tagger\.data/(?P<intent>[^/]+)/((?P<lang>[^/]+)/)?'
)


@dataclass(order=True, frozen=True)
class ModelInfo:
    intent: str
    lang: str = ''  # Empty string semantically means ru (legacy mode)

    @staticmethod
    def from_path(path: str):
        match = MODEL_PATH_PATTERN.match(path)
        if match is None:
            raise ValueError(f"{path} do not match {MODEL_PATH_PATTERN}")

        intent = match.group('intent')
        lang = match.group('lang')

        if lang is None:
            return ModelInfo(intent)

        return ModelInfo(intent, lang)

    def generate_model_paths(self):
        prefix = f'personal_assistant_model_directory/tagger/tagger.data/{self.intent}'
        if self.lang:
            prefix += f'/{self.lang}'

        return [f'{prefix}/{model_file}' for model_file in MODEL_FILES]


def read_project_yamake(yamake_path):
    """ Convert a ya.make with sandbox resources into a list of commands """
    with open(yamake_path, 'r') as f:
        text = f.read()

    command_pattern = re.compile(r'(#[^\n]*)?\n*([a-zA-Z_]*)\((.*?)\)', re.MULTILINE + re.DOTALL)

    intent_to_resource_id = {}
    dst_to_src_intent_info = {}
    for comment, name, values in re.findall(command_pattern, text):
        values = values.strip()
        element = {
            'name': name
        }
        if comment:
            element['comment'] = comment
        if name == 'FROM_SANDBOX':
            values_lines = values.split('\n')
            resource_id = values_lines[0].rstrip()
            names = {'other': [], 'src': [], 'dst': []}
            name_type = 'other'
            for line in values_lines[1:]:
                if line.strip() == 'OUT':
                    name_type = 'dst'
                    continue
                if line.strip() == 'RENAME':
                    name_type = 'src'
                    continue
                names[name_type].append(line.strip())
            assert not names['src'] or len(names['src']) == len(names['dst'])
            for src_name, dst_name in itertools.zip_longest(names['src'], names['dst']):
                dst_intent_info = ModelInfo.from_path(dst_name)
                intent_to_resource_id[dst_intent_info] = resource_id
                if src_name:
                    src_intent_info = ModelInfo.from_path(src_name)
                    dst_to_src_intent_info[dst_intent_info] = src_intent_info

    return intent_to_resource_id, dst_to_src_intent_info


def get_intents_to_update_from_path(path):
    intents_to_update = []
    with os.scandir(path) as dir_contents:
        for intent_dir in dir_contents:
            if not intent_dir.is_dir():
                continue

            languages = [d.name for d in os.scandir(intent_dir.path) if d.is_dir()]
            languages.append('')  # empty dir to include model stored in intent_dir (legacy)

            for language in languages:
                language_dir = os.path.join(intent_dir.path, language)

                model_files_exists = [
                    os.path.exists(os.path.join(language_dir, model_file))
                    for model_file in MODEL_FILES
                ]

                if all(model_files_exists):
                    intents_to_update.append(ModelInfo(intent_dir.name, language))
                elif any(model_files_exists):
                    raise ValueError(f'model directory {language_dir} must include all files: {MODEL_FILES}')

    return intents_to_update


def get_intents_to_update_from_vins_chunks(path_to_vins_chunks_file):
    with open(path_to_vins_chunks_file) as f:
        chunks = yaml.load(f)
    intent_to_resource_id = {}

    for chunk_name, chunk in chunks.items():
        if not chunk_name.startswith('tagger/tagger.data'):
            continue
        if 'is_updated' in chunk and chunk.get('is_updated', False):
            for path in chunk['files']:
                intent_name = path.split('/')[2]
                intent_to_resource_id[ModelInfo(intent_name)] = chunk['resource_id']

    return intent_to_resource_id


def get_sandbox_resource_pieces(intent_to_resource_id, dst_to_src_intent_info):
    assert (not dst_to_src_intent_info or
            sorted(intent_to_resource_id.keys()) == sorted(dst_to_src_intent_info.keys()))
    resource_id_to_intents = {resource_id: [] for _, resource_id in intent_to_resource_id.items()}
    for intent_name, resource_id in intent_to_resource_id.items():
        resource_id_to_intents[resource_id].append(intent_name)
    for resource_id in resource_id_to_intents:
        resource_id_to_intents[resource_id].sort()
    sandbox_resources = []
    for resource_id in sorted(resource_id_to_intents, key=lambda id_: resource_id_to_intents[id_][0]):
        model_file_paths = []
        if dst_to_src_intent_info:
            model_file_paths.append('RENAME')
            for intent_name in sorted(resource_id_to_intents[resource_id]):
                model_file_paths.extend(dst_to_src_intent_info[intent_name].generate_model_paths())
        model_file_paths.append('OUT')
        for intent_name in sorted(resource_id_to_intents[resource_id]):
            model_file_paths.extend(intent_name.generate_model_paths())
        sandbox_resources.append(RESOURCE_TEMPLATE.format(
            resource_id=resource_id,
            model_file_paths='\n'.join(f'    {file_path}' for file_path in model_file_paths)
        ))
    return sandbox_resources


def write_project_yamake(intent_name_to_resource_id, intent_dst_name_to_src_name, yamake_filename):
    without_renames = {k: intent_name_to_resource_id[k] for k in intent_name_to_resource_id if k not in intent_dst_name_to_src_name}
    with_renames = {k: intent_name_to_resource_id[k] for k in intent_dst_name_to_src_name}
    pieces_without_renames = get_sandbox_resource_pieces(without_renames, {})
    pieces_with_renames = get_sandbox_resource_pieces(with_renames, intent_dst_name_to_src_name)
    ya_make = YA_MAKE_TEMPLATE.format(sandbox_resources='\n'.join(pieces_without_renames + pieces_with_renames))
    with open(yamake_filename, 'w') as f:
        f.write(ya_make)


@click.command()
@click.option('--tagger-data-yamake', required=True, help='Path to ya.make file of alice tagger data project')
@click.option('--resource-id', help='Sandbox resource id with updated tagger models')
@click.option('--path-to-updated-models', help='Path to the directory with updated tagger models')
@click.option('--path-to-vins-chunks-file', help='Path to vins chunks file')
def main(tagger_data_yamake, resource_id=None, path_to_updated_models=None, path_to_vins_chunks_file=None):
    if (resource_id is None) == (path_to_vins_chunks_file is None):
        click.echo('Exactly one option of `--resource-id` and `--path-to-vins-chunks-file` should be set', err=True)
        click.Abort()

    updated_intent_to_resource_id = {}
    if path_to_vins_chunks_file is not None:
        updated_intent_to_resource_id = get_intents_to_update_from_vins_chunks(path_to_vins_chunks_file)

    intent_to_resource_id, intent_dst_name_to_src_name = read_project_yamake(tagger_data_yamake)

    if path_to_updated_models is not None:
        new_mapping = {}
        for name in get_intents_to_update_from_path(path_to_updated_models):
            new_mapping[name] = updated_intent_to_resource_id.get(name, resource_id)
        updated_intent_to_resource_id = new_mapping

    for name in intent_to_resource_id:
        if name in intent_dst_name_to_src_name and intent_dst_name_to_src_name[name] in updated_intent_to_resource_id:
            intent_to_resource_id[name] = updated_intent_to_resource_id[intent_dst_name_to_src_name[name]]
    intent_to_resource_id.update(updated_intent_to_resource_id)

    write_project_yamake(intent_to_resource_id, intent_dst_name_to_src_name, tagger_data_yamake)

if __name__ == '__main__':
    sys.exit(main())
