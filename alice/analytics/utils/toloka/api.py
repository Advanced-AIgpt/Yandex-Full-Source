#!/usr/bin/env python
# encoding: utf-8
import logging
from datetime import timedelta, datetime

import requests

from utils.auth import choose_credential

ENV_VARIABLES = {
        'sandbox': {
            'api_root': 'https://sandbox.toloka.yandex.ru/api/v1/',
            'ui_root': 'https://sandbox.toloka.yandex.ru/',
            'hitman_env': 'SANDBOX_YANDEX'
        },
        'prod': {
            'api_root': 'https://toloka.yandex.ru/api/v1/',
            'ui_root': 'https://toloka.yandex.ru/',
            'hitman_env': 'PRODUCTION'
        },
        'yang': {
            'api_root': 'https://yang.yandex-team.ru/api/v1/',
            'ui_root': 'https://yang.yandex-team.ru/',
            'hitman_env': 'YANG_PRODUCTION'
        }
    }


class Requester(object):
    def __init__(self, token=None, toloka_env=None, is_sandbox=True):
        self.is_sandbox = is_sandbox
        if toloka_env is None:
            if is_sandbox:
                self.toloka_env = 'sandbox'
            else:
                self.toloka_env = 'prod'
        # toloka_env in ['sandbox', 'prod', 'yang']
        else:
            self.toloka_env = toloka_env
        self.token = choose_credential(token, 'TOLOKA_TOKEN', '~/.toloka/%s_token' % self.toloka_env)
        self.api_root = ENV_VARIABLES[self.toloka_env]['api_root']
        self.ui_root = ENV_VARIABLES[self.toloka_env]['ui_root']

    def fetch(self, handle, params=None):
        url = self.api_root + handle.lstrip('/')
        req = requests.get(url, params, headers=self._make_headers(), verify=False)
        self._check_status(req)
        return req.json()

    def fetch_list(self, handle, params=None, requests_limit=1000):
        params = params.copy()
        if 'sort' not in params:
            params['sort'] = 'id'
        sort_by = params['sort']

        fetched_items = []

        # fetching all items until API tells us that it doesn't 'has_more' elements
        requests_made = 0
        while True:
            response = self.fetch(handle, params)
            fetched_items.extend(response['items'])

            # if the list is ended, exiting the loop
            if not response['has_more']:
                break

            last_item = fetched_items[-1]
            params['{}_gt'.format(sort_by)] = last_item[sort_by]

            requests_made += 1
            if requests_made >= requests_limit:
                raise ValueError(
                    'Fetching list reached limit of {} requests, '
                    'which probably is an infinite loop'.format(requests_limit)
                )

        return {'items': fetched_items, 'has_more': False}

    def send(self, handle, data=None, method='post'):
        url = self.api_root + handle.lstrip('/')
        fn = getattr(requests, method)  # post/put
        req = fn(url, json=data, headers=self._make_headers(), verify=False)
        self._check_status(req)
        return req.json() if req.content else None

    def _check_status(self, req):
        status = req.status_code
        if status < 200 or status >= 300:
            raise UserWarning('Request to Toloka failed (%s): %s' % (status, req.json()))

    def _make_headers(self):
        return {"Content-Type": "application/json; charset=utf-8",
                "Authorization": "OAuth {}".format(self.token)}


class GeneralRequester(Requester):
    def __init__(self, **kwargs):
        super(GeneralRequester, self).__init__(**kwargs)

    def list_all_projects(self, status=None, limit=None):
        params = dict()
        if status is not None:
            params['status'] = status
        if limit is not None:
            params['limit'] = limit

        return self.fetch_list('projects', params)['items']


class ProjectRequester(Requester):
    def __init__(self, prj_id, **kwargs):
        super(ProjectRequester, self).__init__(**kwargs)
        self.prj_id = None if prj_id is None else str(prj_id)
        self.props = None

    def list_open_pools(self):
        return self.fetch_list('pools', {'project_id': self.prj_id, 'limit': 300, 'status': 'OPEN'})['items']

    def list_banned_users(self):
        return self.fetch_list('user-restrictions', {'scope': 'PROJECT', 'project_id': self.prj_id})

    # Получение инфы

    def get_prj_props(self):
        if self.props is None:
            self.props = self.fetch('projects/%s' % self.prj_id)
        return self.props

    def _get_url(self, path, ident):
        if self.prj_id is None:
            raise UserWarning('Create (or get) project first!')

        fmt = '{root}{path}/{ident}'
        return fmt.format(root=self.ui_root, path=path, ident=ident)

    def get_prj_url(self):
        return self._get_url('requester/project', self.prj_id)

    def get_field_names(self):
        props = self.get_prj_props()
        inp = props['task_spec']['input_spec'].keys()
        out = props['task_spec']['output_spec'].keys()
        return inp, out

    # Редактирование

    def create_prj(self, props):
        if self.prj_id is not None:
            raise UserWarning('Project is already created, id=%s' % self.prj_id)
        self.props = props
        response = self.send('projects', props)
        logging.debug('CREATE: %s', response)
        self.prj_id = response['id']
        return response

    def update_prj(self, new_props):
        if self.prj_id is None:
            raise UserWarning('Project is not created yet')
        response = self.send('projects/%s' % self.prj_id, new_props, method='put')
        self.props = new_props
        return response

    def patch_prj(self, patch_fn):
        props = self.get_prj_props()
        patch_fn(props)
        self.update_prj(props)


class PoolRequester(Requester):
    def __init__(self, prj_id, pool_id, **kwargs):
        super(PoolRequester, self).__init__(**kwargs)
        self.prj_id = None if prj_id is None else str(prj_id)
        self.pool_id = None if pool_id is None else str(pool_id)

    # Получение инфы

    def list_pool_tasks(self):
        tasks = self.fetch_list('tasks', {'pool_id': self.pool_id, 'limit': 100000})['items']

        if not tasks:
            # В пуле без миксера список тасок пуст и их можно вытащить только из готовых наборов.
            seen_ids = set()
            for s in self.list_task_suites():
                for t in s['tasks']:
                    if t['id'] not in seen_ids:
                        seen_ids.add(t['id'])
                        tasks.append(t)

        return tasks

    def list_task_suites(self):
        return self.fetch_list('task-suites', {'pool_id': self.pool_id, 'limit': 10000, 'sort': 'id'})['items']

    def list_assignments(self):
        # TODO: Ответ слишком запутанный. Надо бы сделать совместимым с хитмановским get assignments
        return self.fetch_list('results', {'pool_id': self.pool_id, "status": "SUBMITTED,ACCEPTED"})

    def get_pool_props(self):
        return self.fetch('pools/%s' % self.pool_id)

    def get_url(self):
        if self.pool_id is None or self.prj_id is None:
            raise UserWarning('Pool is not created yet')

        fmt = '{0.ui_root}requester/project/{0.prj_id}/pool/{0.pool_id}'
        return fmt.format(self)

    def get_env_name(self):
        return ENV_VARIABLES[self.toloka_env]['hitman_env']

    # Редактирование

    def create_pool(self):
        if self.pool_id is not None:
            raise UserWarning('Pool is already created, id=%s' % self.pool_id)
        props = self._make_pool_props()
        response = self.send('pools', props)
        logging.debug('CREATE: %s', response)
        self.pool_id = response['id']
        return response

    def update_pool(self):
        if self.pool_id is None:
            raise UserWarning('Pool is not created yet')
        props = self._make_pool_props()
        return self.send('pools/%s' % self.pool_id, props, method='put')

    def _make_pool_props(self):
        raise NotImplementedError

    @staticmethod
    def ttl_to_expire(days=365, hours=0, minutes=0):
        expire = datetime.now() + timedelta(days=days, hours=hours, minutes=minutes)
        return expire.isoformat()

    def open_pool(self):
        if self.pool_id is None:
            raise UserWarning('Pool is not created yet')
        return self.send('pools/%s/open' % self.pool_id)

    def close_pool(self):
        if self.pool_id is None:
            raise UserWarning('Pool is not created yet')
        return self.send('pools/%s/close' % self.pool_id)

    def upload_task_suites(self, suites, infinite_overlap=False):
        """
        Загрузка наборов заданий в пул
        :param suites: Список листов. Каждый лист - список заданий в таком формате:
           {"input_values": {}, "solution": {}, "hint": ""}
        :return:
        """
        if self.pool_id is None:
            raise UserWarning('Pool is not created yet')

        def make_task(task):
            t = {'input_values': task['input_values']}
            if task.get('solution'):
                t['known_solutions'] = [{'output_values': task['solution']}]
            if task.get('hint'):
                t['message_on_unknown_solution'] = task['hint']
            return t

        to_load = []
        for s in suites:
            to_load.append({'pool_id': self.pool_id,
                            'tasks': map(make_task, s),
                            'infinite_overlap': infinite_overlap})
        self.send('task-suites?allow_defaults=true', to_load)
