import logging
import os

import click
from aiohttp import web

logging.basicConfig(level=logging.DEBUG)

KNOWN_BOTS = set()


def fallback(request: web.Request):
    return web.HTTPFound(location='https://oauth.yandex.ru/verification_code?' + request.query_string)


def add_known_bot(name: str):
    KNOWN_BOTS.add(name.lower())


def is_known_bot(bot: str):
    return bot is not None and bot.lower() in KNOWN_BOTS


async def handle(request: web.Request):
    bot = request.query.get('state')
    code = request.query.get('code')
    if not is_known_bot(bot) or code is None:
        return fallback(request)
    return web.HTTPFound(location=f'https://t.me/{bot}?start={code}')


@click.command()
@click.option('-p', '--port', type=int, help='port', default=8080)
@click.option('-h', '--host', type=str, help='host', default='::')
def main(port, host):
    bots = os.environ.get('BOTS', '').split(',')
    for bot in ['EloiseHawkingBot'] + bots:
        add_known_bot(bot)
    app = web.Application()
    app.add_routes([
        web.get('/oauth/token', handle),
    ])
    web.run_app(app, host=host, port=port)


if __name__ == '__main__':
    main()
