import asyncio
import json
from asyncio import Semaphore

from aiohttp import ClientSession, ContentTypeError

from alice.tools.nanny_tool.lib.tools import get_logger

log = get_logger(__name__)


async def get(url: str, session: ClientSession) -> (str, json):
    async with session.get(url) as response:
        if response.status != 200:
            log.error(f'{url} failed {response.status}')
            return None
        try:
            return await response.json()
        except ContentTypeError:
            return await response.text()


async def bound_get(sem: Semaphore, url: str, session: ClientSession):
    tasks = []
    async with sem:
        task = asyncio.ensure_future(get(url, session))
        tasks.append(task)
    return await asyncio.gather(*tasks)


async def post(url: str, payload: json, session: ClientSession) -> (str, json):
    async with session.post(url, data=payload, headers={'Content-Type': 'application/json'}) as response:
        try:
            return await response.json()
        except ContentTypeError:
            return await response.text()


async def put(url: str, payload: json, session: ClientSession) -> (str, json):
    async with session.put(url, json=payload, headers={'Content-Type': 'application/json'}) as response:
        try:
            return await response.json()
        except ContentTypeError:
            return await response.text()
