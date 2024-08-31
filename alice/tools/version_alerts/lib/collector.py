import asyncio
import os
import time
from typing import Dict, Tuple, List, Optional, Union
from urllib.parse import urljoin

import aiohttp
from aiohttp import ServerDisconnectedError, ClientConnectionError

from alice.tools.version_alerts.lib.common import make_logger

log = make_logger('Collector')

_NANNY_RESOURCES_TEMPLATE = 'https://nanny.yandex-team.ru/v2/services/{}/runtime_attrs/resources/'
_NANNY_INSTANCES_TEMPLATE = 'https://nanny.yandex-team.ru/v2/services/{}/current_state/instances/partial'
_DEFAULT_TIMEOUT = aiohttp.ClientTimeout(total=5 * 60, connect=None)
_SIMULTANEOUS_REQUESTS = os.environ.get('SIM_REQUESTS', 5)


class Collector:
    def __init__(self, ttl: int, services: Dict):
        self._TOKEN: str = os.environ.get('NANNY_TOKEN')
        self._ttl: int = ttl
        self._services: Dict = services
        self._collected_data: Dict = {service: {'service_versions': None, 'resource_versions': None} for service in
                                      self._services}
        self._nanny_connect_timeout: int = 30
        self._last_gather_time: int = int(time.time())
        self._last_collecting_time: int = 0
        self.is_started: bool = False
        if not self._TOKEN:
            log.warning('NANNY_TOKEN is not set')
        log.info(f'Listening services: {", ".join(self._services.keys())}')

    async def run(self):
        try:
            await self.worker_cycle()
        except asyncio.CancelledError:
            log.info('Job cancelled. Shutting down')
        except Exception as e:
            log.error(f'Got exception: {e}')
            log.exception(e)
        finally:
            quit()

    async def worker_cycle(self):
        while True:
            start_time = int(time.time())
            log.info(f'Gathering started @ {start_time}')
            await self.gather_and_store_data()
            self._last_gather_time = int(time.time())
            self._last_collecting_time = self._last_gather_time - start_time
            log.info(f'Gathering finished @ {self._last_gather_time}: {self._last_collecting_time} seconds')
            self.is_started = True
            await asyncio.sleep(self._ttl)

    async def raw_data(self):
        return self._collected_data

    async def report(self) -> List:
        return await self.generate_report()

    async def generate_report(self) -> List:
        result = []
        for service, values in self._collected_data.items():
            max_version = 0
            beta_for = self._services[service]['beta_for']
            if beta_for is not None:
                if beta_for not in self._services:
                    log.error(f'{service} is beta for {beta_for}, but {beta_for} is not monitored')
                    continue
                log.debug(f'Checking {service} consistency\n{values}\n{self._collected_data[beta_for]}')
                consistent_value = self._consistency_level(values['service_versions'],
                                                           self._collected_data[beta_for]['service_versions'])
                if values.get('resource_versions', None):
                    if not self._collected_data[beta_for].get('resource_versions'):
                        log.error(f'{service} has version check defined, but {beta_for} doesn\'t. Fixing')
                        self._services[beta_for]['check_resources'] = self._services[service]['check_resources']
                    consistent_value += self._compare_resources(values['resource_versions'],
                                                                self._collected_data[beta_for]['resource_versions'])
                result.append(
                    [f'{service.replace("_", "-")}-consistent_axxx',
                     consistent_value]
                )
            log.debug(f'Checking {service} inner consistency')
            try:
                inner_consistency = self._list_consistency(values['service_versions'])
                max_version = max(values['service_versions'])
            except ValueError as e:
                log.info(f'Inner consistency for {service} NOT met: {e}')
                inner_consistency = 1.0
            except TypeError as e:
                log.info(f'Skipping version_report for {service}: {e}')
                continue
            result.append([f'{service.replace("_", "-")}_axxx', inner_consistency])
            result.append([f'{service.replace("_", "-")}-version_axxx', max_version])
        result.append(['elderness_axxx', int(time.time()) - self._last_gather_time])
        result.append(['collection-speed_axxx', int(self._last_collecting_time)])
        return result

    async def gather_and_store_data(self):
        async with aiohttp.ClientSession(headers={'Authorization': f'OAuth {self._TOKEN}'}) as nanny_session:
            tasks = [self.collect_service_data(service, nanny_session)
                     for service in self._services]
            service_results = await asyncio.gather(*tasks)
            await self._store_data(service_results)

    async def collect_service_data(self, service_id: str, nanny_session: aiohttp.ClientSession) -> Optional[Dict]:
        log.debug(f'Collecting for {service_id}')
        service = self._services[service_id]
        resources = None

        if service.get('check_resources', None):
            log.debug(f'Service {service} now must check resources')
            resources = await self._get_resources_versions(service_id, nanny_session)
        if service.get('check_resources_only', False):
            log.debug(f'Service {service} is set to resources only')
            return {'service': service_id,
                    'versions': (1,),
                    'resources': resources}

        try:
            resolved_instances = await self._resolve_instances(nanny_session, service_id)
        except TimeoutError:
            log.error(f'Request timeout: {service_id}')
            return
        except AssertionError:
            log.error(f'No instances resolved in {service_id}')
            return
        except ServerDisconnectedError:
            log.error(f'Timeout on {service_id}')
            return

        versions_by_instances = await self.collect_versions_from_instances(resolved_instances, service, service_id)

        log.info(
            f'Finished collecting {service_id}. Collected {len(versions_by_instances)} from {len(resolved_instances)} instances')
        return {'service': service_id,
                'versions': versions_by_instances,
                'resources': resources}

    async def collect_versions_from_instances(self, resolved_instances: List, service: Dict, service_id: str) -> List:
        tasks = [
            self._get_version_from_instance(instance, service['url'], service['token'], service_id,
                                            service['params'], service['timeout']) for
            instance in resolved_instances]
        result = []
        split_tasks = [tasks[x:x + _SIMULTANEOUS_REQUESTS] for x in range(0, len(tasks), _SIMULTANEOUS_REQUESTS)]
        for tasks_chunks in split_tasks:
            result += await asyncio.gather(*tasks_chunks)
        result = [x for x in result if x != -1]
        if len(result) == 0:
            log.error(f'Was unable to fetch {service_id}')
            result = [0, ]
        return result

    async def _resolve_instances(self, nanny_session: aiohttp.ClientSession, service_id: str) -> List:
        resolved_instances = await asyncio.wait_for(
            self._get_service_instances_from_nanny(service_id, nanny_session),
            timeout=self._nanny_connect_timeout)
        assert len(resolved_instances) > 0
        return resolved_instances

    async def _store_data(self, data_to_store: List[Dict]):
        for service_data in data_to_store:
            if not service_data:
                continue
            validated_data = []

            service = self._services[service_data['service']]
            if self._services[service_data['service']]['parser'] is not None:
                log.debug(f"Calling additionаl parser {service['parser']}")
                parse_task = [service['parser'].parse(value) for value in service_data['versions']]
                unvalidated_data = await asyncio.gather(*parse_task)
            else:
                unvalidated_data = service_data['versions']
            for value in unvalidated_data:
                try:
                    validated_data.append(int(value))
                except (ValueError, TypeError):
                    log.error(f'Using default "0" instead of bad "{value}" from {service_data[0]}'.replace('\n', ' '))
                    validated_data.append(0)
            self._collected_data[service_data['service']]['service_versions'] = validated_data
            if 'resources' in service_data:
                self._collected_data[service_data['service']]['resource_versions'] = service_data['resources']

    @staticmethod
    async def _get_service_instances_from_nanny(service_id: str, nanny_session: aiohttp.ClientSession) -> List:
        try:
            async with nanny_session.get(_NANNY_INSTANCES_TEMPLATE.format(service_id)) as resp:
                data = await resp.json()
                assert 'instancesPart' in data
        except AssertionError:
            log.error(f'No instances in {service_id}')
            return []
        except ClientConnectionError as e:
            log.error(f'{service_id}: unable to resolve instances: {e}')
        result = [x['container_hostname'] for x in data['instancesPart'] if 'container_hostname' in x]
        if 'errors' in data and len(data['errors']) > 0:
            log.error(f'Error in resolving {service_id} instances: {[x["error"] for x in data["errors"]]}')
        return result

    async def _get_resources_versions(self, service_id: str, nanny_session: aiohttp.ClientSession) -> Dict:
        try:
            async with nanny_session.get(_NANNY_RESOURCES_TEMPLATE.format(service_id)) as resp:
                data = await resp.json()
            assert 'content' in data
            assert 'sandbox_files' in data['content']
        except AssertionError:
            log.error(f"Resource request returned no content for {service_id}")
            return {}
        except ClientConnectionError as e:
            log.error(f'Unable to collect resource data for {service_id}: {e}')
            return {}
        try:
            result = {x['local_path']: int(x['resource_id'])
                      for x in data['content']['sandbox_files'] if
                      x['local_path'] in self._services[service_id]['check_resources']}
        except ValueError:
            log.error(f'Bad value for a resource in {service_id}')
            return {}
        return result

    @staticmethod
    async def _get_version_from_instance(host: str, url: str = '/ping', token: str = None, service_id: str = None,
                                         params: Dict = None,
                                         timeout: int = None) -> Union[str, int]:
        if not host.startswith('http'):
            host = f'http://{host}'
        headers = {'Authorization': f'OAuth {token}'} if token else None
        request_url = urljoin(host, url)

        if timeout and type(timeout) is int:
            connect_timeout = aiohttp.ClientTimeout(total=5 * 60, connect=timeout)
        else:
            connect_timeout = _DEFAULT_TIMEOUT

        async with aiohttp.ClientSession(connector=aiohttp.TCPConnector(force_close=True),
                                         headers=headers,
                                         timeout=connect_timeout) as session:
            try:
                async with session.get(request_url, params=params) as resp:
                    assert resp.status == 200
                    data = await resp.text()
            except AssertionError:
                log.error(f'{service_id}: {host} returned {resp.status}')
                data = 0
            except ClientConnectionError as e:
                log.error(f'{service_id}: Unable to connect to {host} ({e})')
                data = -1
        return data

    @staticmethod
    def _consistency_level(beta_versions: Tuple, prod_versions: Tuple) -> float:
        if beta_versions is None or prod_versions is None:
            return 1.0
        beta = max(beta_versions)
        prod = max(prod_versions)
        if beta != prod:
            log.debug(f'NOT consistent. Beta {beta} != Prod {prod}')
            return 1.0
        try:
            inner_consistency = Collector._list_consistency(beta_versions + prod_versions)
        except ValueError as e:
            log.error(f"Could not match beta and prod, {e}")
            inner_consistency = 1.0
        return inner_consistency

    @staticmethod
    def _compare_resources(beta_resources: Dict, prod_resources: Dict) -> int:
        result = 0
        if beta_resources is None or prod_resources is None:
            return 1
        log.debug(f'Comparing {beta_resources} and {prod_resources}')
        for k in beta_resources:
            result += prod_resources.get(k, 0) - beta_resources[k]
        if abs(result) > 0:
            return 1
        return 0

    @staticmethod
    def _list_consistency(values: Tuple) -> float:
        if not values:
            log.info('Inner consistency NOT met: no values')
            return 1.0

        max_version = abs(max(values))
        if max_version == 0:
            log.info('Inner consistency NOT met: maximum version is 0')
            return 1.0

        result = 1 - (sum(values) / len(values) / max_version)
        if result > 1.0:
            result = 1.0
        log.debug(f'Consistent for {result}: {values}')
        return result
