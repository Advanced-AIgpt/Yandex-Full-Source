import logging
import click
import mode_handler
import mode_configs
import cProfile
import numpy as np

@click.command()
@click.option(
    '--config-path',
    type=click.Path(exists=True),
    required=True,
    help="Path to config. You can find examples at alice/nlu/data/train.")
@click.option(
    '--profile',
    type=click.Path(),
    help="Path to file where to save profiled data.")
def main(config_path, profile):
    logging.basicConfig(level=logging.INFO, format=u'\u001b[35;1m[%(asctime)s]\u001b[36m [%(pathname)s:%(lineno)d] [%(levelname)s]\u001b[0m\n%(message)s\n')

    if profile is not None:
        logging.info('Profiled data will be saved at {}'.format(profile))
        pr = cProfile.Profile()
        pr.enable()

    config = mode_configs.Config.from_json(config_path)

    if config.mode == 'fit':
        mode_handler.handle_fit_mode(config)

    elif config.mode == 'predict':
        mode_handler.handle_predict_mode(config)

    if profile is not None:
        pr.disable()
        pr.dump_stats(profile)
