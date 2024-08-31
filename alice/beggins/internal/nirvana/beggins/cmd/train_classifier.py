import logging

import click
import numpy
import tensorflow

from beggins.lib import container

from beggins.lib.config import read_config
from beggins.lib.dataset import load_datasets
from beggins.lib.evaluation import evaluate
from beggins.lib.export import export
from beggins.lib.keras_tools import make_model
from beggins.lib.learning import train

logging.basicConfig(format='%(asctime)s [%(levelname)s] [%(name)s]: %(message)s', level=logging.DEBUG)
logger = logging.getLogger(__name__)


def init_env():
    numpy.random.seed(42)
    tensorflow.set_random_seed(42)


@click.command()
@click.option('-c', '--config', required=True)
def main(config):
    init_env()
    config = read_config(config, container.FORMAT_MAP)
    logger.info(f'Config: {config}')
    model = make_model(config.model)
    datasets = load_datasets(config.datasets)
    train(config.learning_stages, model, datasets)
    evaluate(config.eval, datasets)
    export(config.export)


if __name__ == '__main__':
    main()
