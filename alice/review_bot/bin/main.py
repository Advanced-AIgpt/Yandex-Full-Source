# coding: utf-8

import click

from alice.review_bot.ybot.core import serve


@click.command()
@click.argument('config', required=True, type=click.File())
def main(config):
    serve(config)
