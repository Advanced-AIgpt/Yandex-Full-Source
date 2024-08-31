# coding: utf-8
from __future__ import absolute_import

import copy
import json
import hashlib
import logging
import os
import re
import subprocess
from collections import defaultdict, Mapping, OrderedDict

from vins_core.utils.datetime import utcnow
from vins_core.utils.data import (
    get_checksum,
    get_resource_full_path,
    clean_dir,
    load_data_from_file,
)


logger = logging.getLogger(__name__)


def compile_chunks_config(config):
    if isinstance(config, Mapping):
        if 'patterns_file' in config:
            config['patterns'] = read_chunks_config(config['patterns_file'])
        for child_name, child_config in config.get('children', {}).iteritems():
            compile_chunks_config(child_config)


def read_chunks_config(filename):
    config = load_data_from_file(filename)
    compile_chunks_config(config)
    return config


def _get_files(node):
    if os.path.isfile(node):
        return [node]
    results = []
    for parent, dirnames, filenames in os.walk(node, followlinks=True):
        for filename in sorted(filenames):
            results.append(os.path.join(parent, filename))
    return sorted(results)


def dir_to_chunks(current_dir, config, depth=0, expand=False):
    """
    Traverse the directory and get list of chunks covering all its content according to the config.
     - by default, if shallow (depth=0), convert the whole dir into a chunk
     - if there are children, apply the function to each child, and default logic to the rest of the contents
     - if there are patterns and/or prune_suffix_by, chunk contents by them, and apply default logic to the rest
    """
    if config.get('ignore', False):
        return []
    if os.path.isfile(current_dir):
        return [{
            'name': current_dir,
            'files': [current_dir]
        }]
    if 'depth' in config:
        depth = int(config['depth'])
    depth = max(0, depth)
    children = config.get('children', {})
    patterns = config.get('patterns', [])
    for p in patterns:
        p['compiled_pattern'] = re.compile(p['pattern'])
    prune_suffix_by = config.get('prune_suffix_by')
    if depth == 0 and not children and not patterns and not prune_suffix_by:
        # for ya.make, we need final filenames instead of just directory names
        if expand:
            files = _get_files(current_dir)
        else:
            files = [current_dir]
        return [{
            'name': current_dir,
            'files': sorted(files)
        }]
    chunks = []
    candidates = sorted(os.listdir(current_dir))
    named_patterns = defaultdict(list)
    for candidate in candidates:
        if candidate in children:
            for chunk in dir_to_chunks(os.path.join(current_dir, candidate), children[candidate], depth - 1, expand):
                chunks.append(chunk)
        else:
            matched = False
            for pattern in patterns:
                if re.match(pattern['compiled_pattern'], candidate):
                    matched = True
                    for file in _get_files(os.path.join(current_dir, candidate)):
                        named_patterns[pattern['name']].append(file)
                    break
            if not matched and prune_suffix_by:
                matched = True
                for file in _get_files(os.path.join(current_dir, candidate)):
                    named_patterns[candidate.split(prune_suffix_by)[0]].append(file)
            if not matched:
                for chunk in dir_to_chunks(os.path.join(current_dir, candidate), {}, depth - 1, expand):
                    chunks.append(chunk)
    for pattern_name, pattern_files in named_patterns.iteritems():
        chunks.append({
            'name': os.path.join(current_dir, pattern_name),
            'files': sorted(pattern_files)
        })
    assert len(chunks) == len({chunk['name'] for chunk in chunks}), 'chunk names are not unique'
    return sorted(chunks, key=lambda x: x['name'])


def format_chunks(chunks, root_dir):
    chunks = copy.deepcopy(chunks)
    result = OrderedDict()
    for chunk in chunks:
        chunk['name'] = os.path.relpath(chunk['name'], root_dir)
        hashes = '_'.join([get_checksum(f) + '_' + os.path.relpath(f, root_dir) for f in sorted(chunk['files'])])
        chunk['sha256'] = hashlib.sha256(hashes).hexdigest()
        chunk['files'] = [os.path.relpath(fn, root_dir) for fn in chunk['files']]
        chunk['filename'] = chunk['name'].replace('/', '.')
        result[chunk['name']] = chunk
    return result


def compare_chunks(new_chunks, old_chunks=None):
    now = utcnow().strftime('%Y-%m-%d %H:%M:%S')
    old_chunks = old_chunks or {}  # create a stub for the first training run
    for chunk_key, new_chunk in new_chunks.iteritems():
        old_chunk = old_chunks.get(str(chunk_key), {})
        if old_chunk.get('sha256') != new_chunk.get('sha256'):
            new_chunk['is_updated'] = True
            new_chunk['last_update'] = now
        else:
            new_chunk['is_updated'] = False
            new_chunk['last_update'] = old_chunk.get('last_update')
            if 'resource_id' in old_chunk and old_chunk['resource_id']:
                new_chunk['resource_id'] = old_chunk.get('resource_id')
            if 'task_id' in old_chunk and old_chunk['task_id']:
                new_chunk['task_id'] = old_chunk.get('task_id')


def upload_dir_to_sandbox(dirname, compress=False, token=None, owner=None):
    yatool = get_resource_full_path('resource://arcadia/ya')
    subprocess.check_call(['chmod', 'a+x', yatool])
    command = [yatool, 'upload', '--do-not-remove', '--json-output', dirname]
    if compress:
        command.append('--tar')
    if token:
        command.extend(['--token', token])
    if owner:
        command.extend(['--owner', owner])
    output = subprocess.check_output(command)
    return json.loads(output)


def read_resources_yamake(yamake_filename):
    """ Convert a ya.make with sandbox resources into a list of commands """
    with open(yamake_filename, 'r') as f:
        text = f.read()
    pattern = re.compile(r'(#[^\n]*)?\n*([a-zA-Z_]*)\((.*?)\)', re.MULTILINE + re.DOTALL)
    elements = []
    while True:
        text = text.strip()
        m = re.search(pattern, text)
        if not m:
            break
        text = text[m.end():]
        comment, key, values = m.groups()
        values = values.strip()
        element = {
            'key': key
        }
        if comment:
            element['comment'] = comment
        if key == 'FROM_SANDBOX':
            values_lines = [line.strip() for line in values.split('\n')]
            element['resource_id'] = values_lines.pop(0)
            if values_lines[0] == 'FILE':
                element['file'] = True
                values_lines.pop(0)
            if values_lines[0] == 'PREFIX':
                values_lines.pop(0)
                element['prefix'] = values_lines.pop(0)
            assert values_lines.pop(0) == 'OUT'
            element['out'] = values_lines
        elif values:
            element['data'] = values
        elements.append(element)
    assert not text
    return elements


def write_resources_yamake(elements, yamake_filename):
    """ Convert a list of ya.make commands about sandbox resources into a ya.make file """
    text = ''
    for element in elements:
        if element.get('comment'):
            text += '{}\n'.format(element['comment'])
        text += element['key'] + '('
        if 'data' in element:
            text += element['data']
        elif element['key'] == 'FROM_SANDBOX':
            text += '\n'
            text += '    {}\n'.format(element['resource_id'])
            if element.get('file'):
                text += '    FILE\n'
            if element.get('prefix'):
                text += '    PREFIX\n'
                text += '    {}\n'.format(element['prefix'])
            text += '    OUT\n'
            for filename in element['out']:
                text += '    {}\n'.format(filename)
        text += ')\n\n'
    with open(yamake_filename, 'w') as f:
        f.write(text)


def update_resources_yamake(yamake_filename, model_name, chunks):
    yamake_data = read_resources_yamake(yamake_filename)
    assert yamake_data[0]['key'] == 'UNION'
    end_pos = next(i for i, x in reversed(list(enumerate(yamake_data))) if x.get('key') == "END")
    last_items = yamake_data[end_pos:]
    yamake_data = yamake_data[:end_pos]
    chunk_names = sorted(list(chunks.keys()))

    non_model_resources = [x for x in yamake_data if not (x.get('key') == 'FROM_SANDBOX' and
                                                          x.get('prefix', '').startswith(model_name))]
    new_model_resources = []
    for chunk_name in chunk_names:
        chunk = chunks[chunk_name]
        symlink_prefix = model_name
        resource = {
            'key': 'FROM_SANDBOX',
            'resource_id': str(chunk['resource_id']),
            'prefix': symlink_prefix,
            'out': [symlink_prefix + '/' + filename for filename in chunk['files']]
        }
        new_model_resources.append(resource)
    new_yamake_data = non_model_resources + new_model_resources + last_items
    write_resources_yamake(new_yamake_data, yamake_filename)


def link_chunks_to_dir(chunks, resources_path, model_name, link_to_links=True, clean_the_dir=True):
    # remove the app model directory
    model_dir = os.path.join(resources_path, model_name)
    if clean_the_dir and os.path.exists(model_dir):
        clean_dir(model_dir)
    chunks = chunks or {}  # create a stub for the first training run
    # collect the symlinks for the chunks and create symlinks to each component of chunk from the app directory
    for chunk_name, chunk in chunks.iteritems():
        for filename in chunk['files']:
            if link_to_links:
                # here we rely on the symlinks created automatically by ci tools or the initializer of DirectoryView,
                # without running setup-venv.sh
                src_filepath = os.path.join(
                    resources_path, model_name + '.' + chunk['filename'], chunk['filename'], filename
                )
            else:
                src_filepath = os.path.join(resources_path, chunk['resource_id'] + '_ext', chunk['filename'], filename)
            trg_filepath = os.path.join(resources_path, model_name, filename)
            trg_basedir = os.path.dirname(trg_filepath)
            if not os.path.exists(trg_basedir):
                os.makedirs(trg_basedir)
            if os.path.exists(trg_filepath):
                os.unlink(trg_filepath)
            try:
                os.symlink(src_filepath, trg_filepath)
            except OSError as e:
                raise ValueError(
                    'Could not create symlink from {} to {}: got error {}'.format(src_filepath, trg_filepath, e)
                )
