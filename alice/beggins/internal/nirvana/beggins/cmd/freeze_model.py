import os

import click

from beggins.lib.export import freeze_model


@click.command()
@click.option('--input-filename', required=True)
@click.option('--output-folder', required=True)
def main(input_filename, output_folder):
    os.makedirs(output_folder)
    model_path = os.path.join(output_folder, 'model.pb')
    model_description_path = os.path.join(output_folder, 'model_description.json')
    model_info = freeze_model(input_filename, model_path)
    with open(model_description_path, 'w') as f:
        f.write(model_info.to_json())


if __name__ == '__main__':
    main()
