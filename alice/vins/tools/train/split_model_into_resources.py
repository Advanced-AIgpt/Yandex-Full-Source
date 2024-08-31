#!/usr/bin/env python
# coding: utf-8
import click
import yaml
import logging
import os
import shutil

from vins_core.utils.config import get_setting
from vins_core.utils.data import get_vinsfile_data, load_data_from_file
from vins_core.utils.misc import parallel
from vins_tools.utils.chunks import (
    read_chunks_config, dir_to_chunks, format_chunks, compare_chunks, update_resources_yamake, upload_dir_to_sandbox
)

logger = logging.getLogger(__name__)


def upload_single_chunk(chunk, resources_path, model_dir, initializer=None, **kwargs):
    if not chunk['is_updated'] or 'resource_id' in chunk:
        return chunk
    chunk_dirname = os.path.join(resources_path, chunk['filename'])
    if os.path.exists(chunk_dirname):
        shutil.rmtree(chunk_dirname)
    os.mkdir(chunk_dirname)
    for filename in chunk['files']:
        src_path = os.path.join(model_dir, filename)
        if os.path.isdir(src_path):
            shutil.copytree(src_path, os.path.join(chunk_dirname, filename))
        else:
            trg_dir = os.path.join(chunk_dirname, os.path.dirname(filename))
            if not os.path.exists(trg_dir):
                os.makedirs(trg_dir)
            shutil.copy(src_path, os.path.join(chunk_dirname, filename))
    contents = os.listdir(chunk_dirname)
    if len(contents) != 1:
        raise ValueError('Expected resource dir for {} to have exactly 1 child, got {} instead'.format(
            chunk['name'],
            contents
        ))
    dir_to_upload = os.path.join(chunk_dirname, contents[0])
    print 'uploading... {}'.format(chunk['name'])
    upload_result = upload_dir_to_sandbox(dir_to_upload, **kwargs)
    if upload_result.get('task', {}).get('status') not in {'SUCCESS', 'FINISHING'}:
        raise ValueError('Upload for chunk {} failed with result {}'.format(chunk, upload_result))
    chunk['resource_id'] = str(upload_result['resource_id'])
    chunk['task_id'] = str(upload_result['task']['id'])
    print '  {} : resource {}'.format(chunk['name'], chunk['resource_id'])
    # the following move makes file structure compatible with that after resource download
    resource_root = os.path.join(resources_path, chunk['resource_id'] + '_ext')
    os.makedirs(resource_root)
    shutil.move(chunk_dirname, resource_root)
    return chunk


def upload_chunks_to_sandbox(chunks, model_dir, resources_path, num_procs=None, compress=True,
                             token=None, owner=None):
    upload_results = parallel(
        function=upload_single_chunk,
        items=[chunk for chunk in chunks.values() if chunk['is_updated']],
        function_kwargs={
            'compress': compress,
            'model_dir': model_dir,
            'resources_path': resources_path,
            'token': token,
            'owner': owner,
        },
        filter_errors=True,
        raise_on_error=True,
        num_procs=num_procs
    )
    for chunk in upload_results:
        chunks[chunk['name']] = chunk
    return chunks


def get_original_filename(include):
    """ This is a fix to get the original resource filename from Vinsfile.json,
    in contrast with resolve_include, which now gives only filename of a copy of the resource
    """
    if include.find('personal_assistant') >= 0:
        return 'personal_assistant/config/chunks.yaml'
    elif include.find('crm_bot') >= 0:
        return 'crm_bot/config/chunks.yaml'
    raise NotImplementedError('Don\'t know how to resolve packages other from personal_assistant or crm_bot')


class DictOption(click.Option):
    _value_key = '_default_val'

    def __init__(self, *args, **kwargs):
        self.default_option = kwargs.pop('key_option', None)
        self.default_dict = kwargs.pop('default_dict', {})
        super(DictOption, self).__init__(*args, **kwargs)

    def get_default(self, ctx):
        if not hasattr(self, self._value_key):
            if self.default_option is None or self.default_dict is None:
                default = super(DictOption, self).get_default(ctx)
            else:
                arg = ctx.params[self.default_option]
                default = self.default_dict.get(arg, None)
                if default is None:
                    default = super(DictOption, self).get_default(ctx)
            setattr(self, self._value_key, default)
        return getattr(self, self._value_key)


_default_values = {
    'yamake': {
        'personal_assistant': 'resources/ya.make',
        'crm_bot': 'resources/crm_bot/ya.make'
    },
    'chunks': {
        'personal_assistant': 'apps/personal_assistant/personal_assistant/config/chunks.yaml',
        'crm_bot': 'apps/crm_bot/crm_bot/config/chunks.yaml',
    },
    'model-dir': {
        'personal_assistant': None,
        'crm_bot': 'resources/crm_bot/crm_bot_model_directory'
    }
}


@click.command()
@click.option('--app', help='Application to compile the model for', default='personal_assistant')
@click.option('--yamake', help='ya.make file for sandbox resources',
              cls=DictOption, key_option='app', default_dict=_default_values['yamake'])
@click.option(
    '--chunks',
    help='Config with new model\'s chunks',
    cls=DictOption, key_option='app', default_dict=_default_values['chunks']
)
@click.option(
    '--sandbox-token',
    default=None,
    help='OAuth token for ya upload, if no user rsa private key provided (example: for nirvana)'
)
@click.option('--model-dir', help='Directory with the compiled model',
              cls=DictOption, key_option='app', default_dict=_default_values['model-dir'])
@click.option('--owner', default=None, help='Sandbox resource owner')
@click.option('--no-upload', help='Whether resources upload is not needed', is_flag=True)
@click.option('--no-compare', help='Whether chunks diff computation is not needed', is_flag=True)
@click.option('--keep-updated-flag', is_flag=True, help='Whether to keep `is_updated` flag in chunks file')
def main(app, yamake, chunks, sandbox_token,
         model_dir=None, owner=None, no_upload=False, no_compare=False, keep_updated_flag=False):
    logger.info('Getting configuration info...')
    model_name = app + '_model_directory'
    vinsfile_data = get_vinsfile_data(package=app)
    chunks_filename = vinsfile_data['nlu']['compiled_model']['chunks']
    chunks_final_filename = get_original_filename(chunks_filename)
    chunks_config_filename = vinsfile_data['nlu']['compiled_model']['chunks_config']
    resources_path = get_setting('RESOURCES_PATH', default='/tmp/vins/sandbox')
    if model_dir is None:
        model_dir = os.path.join(resources_path, model_name)

    need_to_compare = not no_compare
    need_to_upload = not no_upload

    old_chunks_file = load_data_from_file(chunks_final_filename)
    if need_to_compare:
        logger.info('Building the chunks config...')
        # open chunks config for the current application (given as an argument)
        chunks_config = read_chunks_config(chunks_config_filename)
        # create the chunks file for the model
        logger.info('Splitting the directory into chunks...')
        chunks_content = dir_to_chunks(model_dir, chunks_config, expand=True)
        new_chunks_file = format_chunks(chunks_content, model_dir)
        logger.info('Comparing the chunks file with the old one...')
        compare_chunks(new_chunks_file, old_chunks_file)
    else:
        logger.info('Using the existing chunks file')
        new_chunks_file = old_chunks_file

    if need_to_upload:
        # copy each new chunk into a separate folder and upload it
        logger.info('Starting uploading to sandbox...')
        new_chunks_file = upload_chunks_to_sandbox(
            new_chunks_file, model_dir, resources_path, token=sandbox_token, owner=owner,
        )
        logger.info('Updating the ya.make with resources...')
        update_resources_yamake(yamake, model_name, new_chunks_file)
        for chunk_name, chunk in new_chunks_file.iteritems():
            if not keep_updated_flag:
                del chunk['is_updated']
    else:
        logger.info('Skipping the upload step')

    logger.info('Updating the chunks file "{}"... '.format(chunks))
    with open(chunks, 'w') as f:
        yaml.safe_dump(dict(new_chunks_file), f, default_flow_style=False, allow_unicode=True)
    logger.info('All done!')


def do_main():
    logging.basicConfig(level=logging.ERROR)
    logger.setLevel(logging.DEBUG)
    main()


if __name__ == '__main__':
    do_main()
