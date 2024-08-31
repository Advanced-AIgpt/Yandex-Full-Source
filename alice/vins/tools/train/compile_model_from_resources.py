#!/usr/bin/env python
# coding: utf-8
import click
import os
from vins_core.utils.config import get_setting
from vins_core.utils.data import get_vinsfile_data, load_data_from_file

from vins_tools.utils.chunks import link_chunks_to_dir


@click.command()
@click.option('--app', help='The application to compile the model for', default='personal_assistant')
def main(app):
    model_name = app + '_model_directory'
    vinsfile_data = get_vinsfile_data(package=app)
    chunks_filename = vinsfile_data['nlu']['compiled_model']['chunks']
    resources_path = get_setting('RESOURCES_PATH', default='/tmp/vins/sandbox')

    chunks = load_data_from_file(chunks_filename)
    link_chunks_to_dir(chunks, resources_path, model_name)

    # the action below should be performed by download_resources.py, but here we repeat it to be sure
    for chunk in chunks.values():
        source = os.path.join(resources_path, chunk['resource_id'] + '_ext')
        target = os.path.join(resources_path, model_name + '.' + chunk['filename'])
        if os.path.exists(target):
            os.remove(target)
        os.symlink(source, target)


if __name__ == '__main__':
    main()
