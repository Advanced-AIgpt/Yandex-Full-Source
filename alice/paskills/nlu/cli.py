# coding: utf-8

import logging
import os

import click

from nlu_service.utils.log import setup_logging
from nlu_service.web import app


logger = logging.getLogger(__name__)


@click.group()
@click.option('--line-logging', is_flag=True)
def cli(line_logging):
    setup_logging(line_logging)


@cli.command()
@click.option('--reload', is_flag=True)
def serve(reload):  # pylint: disable=redefined-builtin
    port = int(os.environ.get('QLOUD_HTTP_PORT', 8000))
    app.serve(port, reload)


if __name__ == '__main__':
    cli()
