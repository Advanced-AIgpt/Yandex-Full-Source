from __future__ import unicode_literals

import asyncio
import json
import os
from typing import List, Dict, Optional
from urllib.parse import urljoin

from aiohttp import ClientSession

from alice.tools.nanny_tool.lib.http_methods import post, put, get
from alice.tools.nanny_tool.lib.tools import make_pretty, get_logger, ask_user_ok, make_service_template, clean_exit

log = get_logger(__name__)

# TODO: This is yaldi.stable common resources.
GENERIC_ATTRS = [
    {
        'task_type': 'UPLOAD_VOICETECH_RESOURCE',
        'task_id': '508824144',
        'resource_id': '1123031832',
        'local_path': 'yaldi-server',
        'resource_type': 'VOICETECH_YALDI_SERVER'
    },
    {
        'task_type': 'BUILD_STATBOX_PUSHCLIENT',
        'task_id': '146741790',
        'resource_id': '341072995',
        'local_path': 'push-client',
        'resource_type': 'STATBOX_PUSHCLIENT'
    },
    {
        'task_type': 'HTTP_UPLOAD_2',
        'task_id': '542789743',
        'resource_id': '1199004266',
        'resource_type': 'VOICETECH_CONTAINER_DEPLOY_BUNDLE ',
        'local_path': 'qloud_format.py',
    }]


def _modify_resources(service: Dict, resource_versions: List) -> Dict:
    snapshot_id = service['snapshot_id']
    content = service['content']
    sandbox = resource_versions + GENERIC_ATTRS
    return {'content': {'services_balancer_config_files': content['services_balancer_config_files'],
                        'url_files': content['url_files'],
                        'static_files': content['static_files'],
                        'template_set_files': content['template_set_files'],
                        'l7_fast_balancer_config_files': content['l7_fast_balancer_config_files'],
                        'sandbox_files': sandbox},
            'comment': 'Auto-setup',
            'snapshot_id': snapshot_id
            }


def get_resource_versions(qloud_app: str = None, filename: str = None) -> List[Dict]:
    if qloud_app:
        log.error('qloud_app not implemented')
        clean_exit(1)
    if filename:
        result = []
        try:
            with open(filename) as f:
                read_data = json.load(f)
        except Exception as e:
            log.error(f"Fatal: {e}")
            clean_exit(1)
        for d in read_data:
            if 'skip' not in d:
                result.append(d)
        return result


def _add_resource_to_dict(services_data: Dict, mode: str, resource_dict: Dict) -> Dict:
    snapshot_id = services_data['snapshot_id']
    content = services_data['content']

    if mode == 'sandbox_files':
        for i in content['sandbox_files']:
            if i['local_path'] == resource_dict['local_path']:
                i['task_type'] = resource_dict['task_type']
                i['task_id'] = resource_dict['task_id']
                i['resource_id'] = resource_dict['resource_id']
                i['resource_type'] = resource_dict['resource_type']
    elif mode == 'static_files':
        for i in content['static_files']:
            if i['local_path'] == resource_dict['local_path']:
                i['content'] = resource_dict['content']
    # TODO: Looks like it is no longer needed
    # elif mode == 'temp':
    #     for i in content['static_files']:
    #         if i['local_path'] == 'supervisord.conf.template' \
    #                 and '{{ BSCONFIG_IDIR }}/super' in i['content'] \
    #                 and '/var/run/super' in i['content']:
    #             log.info(f'Preparing: {service}')
    #             i['content'] = i['content'] \
    #                 .replace('{{ BSCONFIG_IDIR }}/super', './super') \
    #                 .replace('/var/run/super', './super')

    return {'content': content,
            'snapshot_id': snapshot_id,
            'comment': f'Auto-update {resource_dict["local_path"]}'}


class Nanny:
    def __init__(self):
        self._endpoint = 'http://nanny.yandex-team.ru/'
        self._auth_token = os.environ.get('NANNY_TOKEN')
        if not self._auth_token:
            log.error("NANNY_TOKEN not set")
            clean_exit(1, self._session)
        self._copy_instances = False
        self._copy_auth_attrs = True
        self._copy_secrets = False
        self._yav_token = None
        self._default_dc = 'man'
        self.loop = asyncio.get_event_loop()
        self._session = None

    def _make_session(self, cookies=None):
        return ClientSession(headers={'Authorization': f'OAuth {self._auth_token}'}, cookies=cookies)

    def clone_service(self,
                      service: str,
                      category: str,
                      description: str,
                      custom_destination: str = None,
                      copy_secrets=False,
                      skip_dc=False) \
            -> None:
        if not category.endswith('/'):
            category += '/'
        if copy_secrets is True:
            self._yav_token = os.environ.get('YAV_TOKEN')
            if self._yav_token is None:
                log.error('Copy secrets requested but no $YAV_TOKEN provided')
                exit(1)
            self._copy_secrets = True
        if skip_dc is True:
            self._default_dc = None

        template = make_service_template(service)

        events_url = urljoin(self._endpoint, f'v2/services/{service}/copies/')

        requests_list = []
        tasks_to_run = []
        dc_to_deploy = ('sas', 'man', 'vla')

        if self._default_dc is not None:
            if custom_destination:
                dc_to_deploy = [self._default_dc]
                template = custom_destination + '-{}'

            for dc in dc_to_deploy:
                destination_service = template.format(dc)
                if destination_service == service:
                    continue
                requests_list.append(self._make_destination_dict(category, description, destination_service, dc))
        else:
            if custom_destination:
                template = custom_destination
            requests_list.append(self._make_destination_dict(category, description, template, None))

        if len(requests_list) == 0:
            log.error('Nothing to do')
            clean_exit(1, self._session)

        for r in requests_list:
            question = f'Will copy {service} to {r["category"]}{r["id"]} with comment "{r["desc"]}". YP cluster: {r.get("yp_cluster", "Any")}'
            if ask_user_ok(question):
                tasks_to_run.append((events_url, json.dumps(r)))

        if len(tasks_to_run) > 0:
            self.loop.run_until_complete(self._clone(tasks_to_run))
        else:
            log.error('Nothing to do')

    def modify_nanny_resources(self, service: str, resource_versions: List[Dict]) -> None:
        if not resource_versions:
            log.error("No resources provided")
            clean_exit(1, self._session)
        services_data = self.loop.run_until_complete(self._get_runtime(service))[0]

        resource_contents = _modify_resources(services_data, resource_versions)
        modification_results = self.loop.run_until_complete(self._send_resource(service, resource_contents))

        log.debug(make_pretty(modification_results))

    def modify_single_resource(self, service: str, source_file: str = None, mode: str = 'sandbox_files') -> None:
        if source_file is None:
            log.error('No source file provided')
            clean_exit(1, self._session)
        else:
            try:
                with open(source_file) as f:
                    resource_dict: Dict = json.load(f)
            except FileNotFoundError:
                log.error(f'File not found: {source_file}')
                clean_exit(1, self._session)

        results = self.loop.run_until_complete(self._run_resource_modification(service, mode, resource_dict))
        log.info(make_pretty(results))

    def _make_destination_dict(self, category: str, description: str, destination_service: str,
                               dc: Optional[str]) -> Dict:
        dest_dict = {
            'id': destination_service,
            'copy_auth_attrs': self._copy_auth_attrs,
            'copy_instances': self._copy_instances,
            'category': category,
            'desc': description,
        }
        if dc:
            dest_dict['yp_cluster'] = dc.upper()
        if self._copy_secrets:
            dest_dict['copy_secrets'] = self._copy_secrets
            dest_dict['renew_delegation_tokens'] = False
        log.info(dest_dict)
        return dest_dict

    async def _clone(self, tasks_to_run: List) -> None:
        cookies = None
        if self._copy_secrets is True:
            cookies = {'Authorization': self._yav_token}
        async with self._make_session(cookies=cookies) as self._session:
            tasks = [asyncio.ensure_future(post(x[0], x[1], self._session)) for x in tasks_to_run]
            log.info(f'Running {len(tasks)} tasks')

            for response in await asyncio.gather(*tasks):
                log.info(make_pretty(response))

    async def _run_resource_modification(self, service: str, mode: str, resource_dict: Dict):
        async with self._make_session() as self._session:
            services_data_coro = asyncio.gather(asyncio.ensure_future(self._get_runtime(service)))
            services_data = await asyncio.wait_for(services_data_coro, 120)
            if not services_data:
                return
            request = _add_resource_to_dict(services_data[0][0], mode, resource_dict)
            results_coro = asyncio.gather(asyncio.ensure_future(self._send_resource(service, request)))
            results = await asyncio.wait_for(results_coro, 120)
        return results

    async def _get_runtime(self, service: str) -> List[Dict]:
        result = []
        for response in await asyncio.gather(asyncio.ensure_future(
                get(urljoin(self._endpoint, f'/v2/services/{service}/runtime_attrs/resources/'), self._session))):
            result.append(response)
        return result

    async def _send_resource(self, service: str, resources: Dict) -> List:
        result = []
        log.debug(urljoin(self._endpoint, f'/v2/services/{service}/runtime_attrs/resources/'))
        for response in await asyncio.gather(asyncio.ensure_future(
                put(urljoin(self._endpoint, f'/v2/services/{service}/runtime_attrs/resources/'), resources,
                    self._session))):
            result.append(response)
        return result
