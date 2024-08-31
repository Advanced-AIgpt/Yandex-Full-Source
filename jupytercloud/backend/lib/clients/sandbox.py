import json

from traitlets import Unicode
from traitlets.config import SingletonConfigurable

from jupytercloud.backend.lib.util.exc import DiskResourceException

from .http import AsyncHTTPClientMixin


class SandboxClient(SingletonConfigurable, AsyncHTTPClientMixin):
    oauth_token = Unicode(config=True)
    url = Unicode('https://sandbox.yandex-team.ru/api/v1.0', config=True)

    async def request(self, uri, **kwargs):
        url = self.url + '/' + uri.lstrip('/')

        return await self._raw_request(url=url, **kwargs)

    async def get_last_resource(self, resource_type, **kwargs):
        items = await self.get_resources(
            resource_type=resource_type,
            state='READY',
            order='-id',
            limit=1,
            **kwargs,
        )

        if not items:
            raise DiskResourceException(
                f'fetched 0 resources of type={resource_type} from sandbox',
            )

        return items[0]

    async def get_resources(
        self, resource_type,
        *, limit=10, order='-id', state='READY',
        **other,
    ):
        for key, value in other.items():
            if isinstance(value, (dict, tuple, list)):
                other[key] = json.dumps(value)

        filter = {
            'type': resource_type,
            'state': state,
            'order': order,
            'limit': limit,
            **other,
        }
        response = await self.request(
            uri='resource',
            method='GET',
            params=filter,
        )

        result = json.loads(response.body)
        items = result.get('items', [])

        self.log.debug('%d resources found with filter %s', len(items), filter)

        return items

    async def get_tasks(
        self, task_type,
        *, limit=10, order='-id',
        **other,
    ):
        if 'status' in other:
            other['status'] = ','.join(other['status'])

        for key, value in other.items():
            if isinstance(value, (dict, tuple, list)):
                other[key] = json.dumps(value)

        filter = {
            'type': task_type,
            'order': order,
            'limit': limit,
            **other,
        }
        response = await self.request(
            uri='task',
            method='GET',
            params=filter,
        )

        result = json.loads(response.body)
        items = result.get('items', [])

        self.log.debug('%d tasks found with filter %s', len(items), filter)

        return items

    async def create_task(
        self, task_type,
        *, description, custom_fields,
        **other,
    ):
        custom_fields = [
            {'name': k, 'value': v}
            for k, v in custom_fields.items()
        ]

        data = {
            'type': task_type,
            'description': description,
            'custom_fields': custom_fields,
            **other,
        }

        response = await self.request(
            uri='task',
            method='POST',
            data=data,
        )

        result = json.loads(response.body)

        self.log.debug('%s task created with id %s', task_type, result['id'])

        return result

    async def start_task(self, task_id):
        response = await self.request(
            uri='batch/tasks/start',
            method='PUT',
            data=[task_id],
        )

        result = json.loads(response.body)

        self.log.debug('task %s started', task_id)

        return result

    async def stop_task(self, task_id):
        response = await self.request(
            uri='batch/tasks/stop',
            method='PUT',
            data={
                'id': [task_id],
                'comment': 'task stopped by user'
            },
        )

        result = json.loads(response.body)

        self.log.debug('task %s stopped', task_id)

        return result
