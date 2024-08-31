# TODO: Did this for yaldi_stable. Embrace in Nanny.py if it has usage or delete

import asyncio
import os
from typing import Dict, List
from urllib.parse import urljoin

from aiohttp import ClientSession, ContentTypeError

from alice.tools.nanny_tool.lib.http_methods import put
from alice.tools.nanny_tool.lib.tools import get_logger, split_service_name, make_service_template

ENDPOINT = 'https://nanny.yandex-team.ru/'
OAUTH_TOKEN = os.environ.get('NANNY_TOKEN')
log = get_logger(__name__)


def fix_tags_by_service(service: str):
    loop = asyncio.get_event_loop()

    _fix_info_attrs(service, loop)
    _fix_runtime_attrs(service, loop)


def _fix_info_attrs(service: str, loop: asyncio.AbstractEventLoop):
    url_template = urljoin(ENDPOINT, '/v2/services/{}/info_attrs/')
    revision = loop.run_until_complete(asyncio.gather(_get_current_revision(url_template, service)))[0]

    for i in revision:
        if revision[i] is not None:
            revision[i]['content']['labels'] = _update_attrs(i, 'info')

    loop.run_until_complete(asyncio.gather(_set_current_revision(url_template, revision)))


def _fix_runtime_attrs(service: str, loop: asyncio.AbstractEventLoop):
    url_template = urljoin(ENDPOINT, '/v2/services/{}/runtime_attrs/instances')
    revision = loop.run_until_complete(asyncio.gather(_get_current_revision(url_template, service)))[0]

    for i in revision:
        if revision[i] is not None:
            revision[i]['content']['yp_pod_ids']['orthogonal_tags'] = _update_attrs(i, 'runtime')

    loop.run_until_complete(asyncio.gather(_set_current_revision(url_template, revision)))


def _split_service_name(service: str):
    result = dict()
    split_service = split_service_name(service)
    result['srv'] = split_service[0]
    if split_service[1] == 'stable':
        result['ctype'] = 'prod'
    elif split_service[1] == 'prestable':
        result['ctype'] = 'prestable'
    else:
        result['ctype'] = 'test'
        log.info(f'Ctype set as test because it was parsed as {split_service[1]}')
    result['itype'] = 'asr'
    result['geo'] = split_service[-1]
    result['prj'] = '-'.join(split_service[2:-1])
    return result


def _update_attrs(service: str, stage: str):
    values = _split_service_name(service)
    if stage == 'runtime':
        return {'ctype': values['ctype'],
                'itype': values['itype'],
                'metaprj': 'voicetech',
                'prj': '{}-{}'.format(values['srv'], values['prj'])}
    elif stage == 'info':
        return [{'key': 'geo', 'value': values['geo']},
                {'key': 'dc', 'value': values['geo']},
                {'key': 'ctype', 'value': values['ctype']},
                {'key': 'itype', 'value': values['itype']},
                {'key': 'prj', 'value': values['prj']}]
    log.error(f'Unknown stage {stage}')
    exit(1)


async def _get_by_service(url_template: str, service_name: str, session: ClientSession) -> (str, Dict):
    url = url_template.format(service_name)
    async with session.get(url) as response:
        if response.status != 200:
            return service_name, None
        try:
            return service_name, await response.json()
        except ContentTypeError:
            return service_name, await response.text()


async def _get_current_revision(url_template: str, service: str) -> Dict:
    revision = dict()
    service_template = make_service_template(service)
    async with ClientSession(headers={'Authorization': f'OAuth {OAUTH_TOKEN}'}) as session:
        tasks = [asyncio.ensure_future(_get_by_service(url_template, service_template.format(x), session)) for x in
                 ('man', 'vla', 'sas')]
        for response in await asyncio.gather(*tasks):
            revision[response[0]] = response[1]
    return revision


async def _set_current_revision(url_template: str, revision: dict) -> List:
    tasks = []
    result = []
    async with ClientSession(headers={'Authorization': f'OAuth {OAUTH_TOKEN}'}) as session:
        for service in revision:
            if revision[service] is None:
                continue
            data = {'content': revision[service]['content'],
                    'comment': 'Updating tags'}
            if '_id' in revision[service]:
                data['snapshot_id'] = revision[service]['_id']
            elif 'snapshot_id' in revision[service]:
                data['snapshot_id'] = revision[service]['snapshot_id']
            tasks.append(asyncio.ensure_future(put(url_template.format(service), data, session)))
        for r in await asyncio.gather(*tasks):
            result.append(r)
    return result


if __name__ == '__main__':
    fix_tags_by_service('yaldi-stable-sprav-datetime-ru-man')
