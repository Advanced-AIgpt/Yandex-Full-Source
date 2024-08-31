from aiohttp import web
from alice.paskills.penguinarium.util.metrics import sensors


async def solomon_handler(request: web.Request) -> web.Response:
    return web.json_response({
        'sensors': [s async for s in sensors.get_sensors()]
    })


async def ping_handler(request: web.Request) -> web.Response:
    return web.Response()
