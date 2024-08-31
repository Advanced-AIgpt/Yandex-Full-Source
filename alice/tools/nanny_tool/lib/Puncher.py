import asyncio
import json
import os
from typing import List
from urllib.parse import urljoin

from aiohttp import ClientSession

from alice.tools.nanny_tool.lib.http_methods import get, post
from alice.tools.nanny_tool.lib.tools import get_logger, ask_user_ok, make_pretty, clean_exit

log = get_logger(__name__)


class Puncher:
    def __init__(self):
        self._endpoint = 'https://api.puncher.yandex-team.ru/api/dynfw/'
        self._auth_token = os.environ.get('PUNCHER_TOKEN')
        if not self._auth_token:
            log.error("PUNCHER_TOKEN not set")
            clean_exit(1)
        self.loop = asyncio.get_event_loop()
        self._session = None

    def _make_session(self):
        return ClientSession(headers={'Authorization': f'OAuth {self._auth_token}'})

    def copy_puncher_rules(self, source: str, destination: str, comment: str) -> None:
        log.info(f'Copying puncher rules from {source} to {destination}')

        rules_to_move = []
        loop = asyncio.get_event_loop()
        loop.run_until_complete(self._find_new_rules(source, destination, comment, rules_to_move))

        question = 'Will copy rules:\n'
        question += make_pretty(rules_to_move)
        if ask_user_ok(question):
            loop.run_until_complete(self._send_new_rules(rules_to_move))
        else:
            log.error('Cancelled')

    async def _get_existing_rules(self, source: str, rules: List, session: ClientSession) -> None:
        url = urljoin(self._endpoint, f'rules?destination={source}')
        rules_json = await get(url, session)
        for i in rules_json['rules']:
            if i['status'] == 'active':
                rules.append({'machine_name': [x['machine_name'] for x in i['sources']],
                              'protocol': i['protocol'],
                              'locations': i['locations'],
                              'until': i['until'],
                              'ports': i['ports']
                              })

    async def _find_new_rules(self, source: str, destination: str, comment: str, result: List) -> None:
        ex_rules = {'source': [], 'destination': []}

        async with self._make_session() as self._session:
            tasks = [
                asyncio.ensure_future(self._get_existing_rules(source, ex_rules['source'], self._session)),
                asyncio.ensure_future(self._get_existing_rules(destination, ex_rules['destination'], self._session))
            ]
            await asyncio.gather(*tasks)

        for i in ex_rules['source']:
            if i not in ex_rules['destination']:
                result.append(
                    {'request': {
                        'sources': i['machine_name'],
                        'destinations': [destination],
                        'protocol': i['protocol'],
                        'ports': i['ports'],
                        'locations': i['locations'],
                        'until': i['until'],
                        'comment': comment
                    }
                    }
                )

    async def _send_new_rules(self, rules_set: List) -> None:
        url = urljoin(self._endpoint, 'requests')
        tasks = []
        async with self._make_session() as self._session:
            for rule in rules_set:
                task = asyncio.ensure_future(post(url, json.dumps(rule), self._session))
                tasks.append(task)
            for response in await asyncio.gather(*tasks):
                log.info(make_pretty(response))
