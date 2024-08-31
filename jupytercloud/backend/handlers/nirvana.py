import functools
import re

from nirvana_api.json_rpc import RPCException
from tornado import web

from jupytercloud.backend.jupyticket import JupyTicket
from jupytercloud.backend.lib.clients.nirvana import (
    NirvanaClient, add_to_cloned_instance, add_to_cloned_workflow, add_to_draft,
    create_jupyter_workflow,
)

from .base import JCAPIHandler
from .utils import NAME_RE, admin_or_self, json_body


class NirvanaBaseHandler(JCAPIHandler):
    jupyter_block_alias = 'run-jupyter-notebook'

    # https://nirvana.yandex-team.ru/operation/48755dc3-ba52-41de-bce1-64862c2079e9
    get_file_from_arcadia_id = '48755dc3-ba52-41de-bce1-64862c2079e9'

    def get_user_nirvana(self, username=None, oauth_token=None, **kwargs):
        username = username or self.current_user.name
        oauth_token = oauth_token or self.get_user_oauth_token(username)
        return NirvanaClient(
            username=username,
            config=self.settings['config'],
            oauth_token=oauth_token,
            **kwargs,
        )


def catch_nirvana_error(method):
    @functools.wraps(method)
    async def wrapper(*args, **kwargs):
        try:
            return await method(*args, **kwargs)
        except (web.HTTPError, RPCException) as e:
            raise web.HTTPError(500, str(e)) from e

    return wrapper


class NirvanaCheckOAuthHandler(NirvanaBaseHandler):
    @web.authenticated
    @admin_or_self
    async def get(self, name=None):
        username = name or self.current_user.name

        oauth = self.get_user_oauth(username)

        if await oauth.check_scope('nirvana:all', userip=self.request.remote_ip):
            result = {
                'need_oauth': False,
            }
        else:
            result = {
                'need_oauth': True,
                'oauth_url': oauth.get_oauth_url(
                    state={'close': True},
                    minimalistic=True,
                ),
            }

        self.write(result)


class NirvanaWorkflowInfo(NirvanaBaseHandler):
    uuid4_re = r'[a-f0-9]{8}-[a-f0-9]{4}-4[a-f0-9]{3}-[89aAbB][a-f0-9]{3}-[a-f0-9]{12}'
    workflow_re = re.compile(rf'(?P<workflow_id>{uuid4_re})(?:/(?P<instance_id>{uuid4_re}))?')

    def get_workflow_from_url(self, url):
        match = self.workflow_re.search(url)

        if match:
            return match.group('workflow_id'), match.group('instance_id')

        return None, None

    @web.authenticated
    @admin_or_self
    @catch_nirvana_error
    async def get(self, name=None):
        username = name or self.current_user.name
        nirvana = self.get_user_nirvana(username)

        workflow_url = self.get_query_argument('workflow_url')

        workflow_id, instance_id = self.get_workflow_from_url(workflow_url)

        if not workflow_id:
            self.write({
                'success': False,
                'reason': 'Failed to find workflow_id in given url',
            })
            return

        try:
            metadata = await nirvana.get_workflow_meta_data(
                workflow_id=workflow_id,
                workflow_instance_id=instance_id,
            )
        except RPCException as e:
            if len(e.args) > 1 and e.args[0] in (50, 90):
                self.write({
                    'success': False,
                    'reason': e.args[1],
                })
                return
            raise

        try:
            await nirvana.edit_workflow_name_and_description(workflow_id)
        except RPCException as e:
            if len(e.args) > 1 and e.args[0] == 50:
                modify_permission = False
            else:
                raise
        else:
            modify_permission = True

        modifiable = (
            metadata.lifecycleStatus == 'draft'
            and modify_permission
        )

        instance_id = instance_id or metadata.instanceId

        if folder_id := getattr(metadata, 'nsId', None):
            folder_id = str(folder_id)

        result = {
            'success': True,
            'workflow_id': workflow_id,
            'instance_id': instance_id,
            'instance_modifiable': modifiable,
            'instance_cloneable': modify_permission,
            'folder_id': folder_id,
            'quota': metadata.quotaProjectId,
            'status': metadata.lifecycleStatus,
            'url': nirvana.client.get_workflow_url(workflow_id, instance_id),
            **{f: getattr(metadata, f, None) for f in ('name', 'owner', 'updated', 'description')}
        }

        self.write(result)


class NirvanaAddToDraft(NirvanaBaseHandler):
    # XXX Когда голова уже не работала
    # сделал так, чтобы self не биндился на nirvana_action
    nirvana_action = [add_to_draft]

    post_schema = {
        'type': 'object',
        'properties': {
            'arcadia_path': {
                'type': 'string',
            },
            'workflow_id': {
                'type': 'string',
            },
            'arcadia_revision': {  # Non-required in case of outdated extension version
                'type': 'string',
            },
            'jupyticket_id': {  # Non-required in case of outdated extension version
                'type': 'integer',
            },
            'instance_id': {
                'type': 'string',
            },
        },
        'required': [
            'arcadia_path',
            'workflow_id',
            'instance_id',
        ],
        'additionalProperties': False,
    }

    @web.authenticated
    @admin_or_self
    @json_body
    async def post(self, json_data, name=None):
        username = name or self.current_user.name
        nirvana = self.get_user_nirvana(username)

        return await self.do_nirvana_action(username, nirvana, json_data)

    async def do_nirvana_action(self, username, nirvana, json_data):
        if revision := json_data.get('arcadia_revision'):
            if not revision.isdigit():
                raise web.HTTPError(400, f'expected SVN numeric revision, got {revision!r}')
            json_data['arcadia_revision'] = int(revision)

        jupyticket = None
        if jupyticket_id := json_data.pop('jupyticket_id', None):
            jupyticket = JupyTicket.get(
                id=jupyticket_id,
                db=self.jupyter_cloud_db,
                startrek_client=self.startrek_client,
            )
            if not jupyticket:
                raise web.HTTPError(404, f'jupyticket with {jupyticket_id=} does not exists')

        try:
            result = await self.nirvana_action[0](
                nirvana,
                **json_data,
            )
        except RPCException as e:
            # exceeded for creating workflow using {project} project
            if e.args and e.args[0] == 70:
                self.write({
                    'success': False,
                    'reason': str(e.args[1]),
                    'retriable': True,
                })
                return

            raise

        # checking _ids in outer scope to ensure we doesn't forget
        # return them
        workflow_id = result.get('workflow_id')
        instance_id = result.get('instance_id')

        assert workflow_id and instance_id

        if jupyticket:
            jupyticket.link_nirvana_instance(
                workflow_id=workflow_id,
                instance_id=instance_id,
            )

        self.write({
            'url': result['url'],
            'success': True,
        })


class NirvanaNewJupyterWorkflow(NirvanaAddToDraft):
    nirvana_action = [create_jupyter_workflow]

    post_schema = {
        'type': 'object',
        'properties': {
            'project': {  # DEPRECATED
                'type': 'string',
            },
            'arcadia_path': {
                'type': 'string',
            },
            'arcadia_revision': {  # Non-required in case of outdated extension version
                'type': 'string',
            },
            'jupyticket_id': {  # Non-required in case of outdated extension version
                'type': 'integer',
            },
            'folder_id': {  # Non-required in case of outdated extension version
                'type': 'string',
            },
            'quota': {  # Non-required in case of outdated extension version
                'type': 'string',
            },
        },
        'required': [
            'arcadia_path',
        ],
        'additionalProperties': False,
    }

    async def do_nirvana_action(self, username, nirvana, json_data):
        description = (
            f"Workflow created from {self.settings['jupyter_public_host']} "
            f"for notebook {json_data['arcadia_path']}"
        )

        json_data['description'] = description

        return await super().do_nirvana_action(username, nirvana, json_data)


class NirvanaAddToClonedInstance(NirvanaAddToDraft):
    nirvana_action = [add_to_cloned_instance]


class NirvanaAddToClonedWorkflow(NirvanaNewJupyterWorkflow):
    nirvana_action = [add_to_cloned_workflow]

    post_schema = {
        'type': 'object',
        'properties': {
            'arcadia_path': {
                'type': 'string',
            },
            'arcadia_revision': {  # Non-required in case of outdated extension version
                'type': 'string',
            },
            'jupyticket_id': {  # Non-required in case of outdated extension version
                'type': 'integer',
            },
            'workflow_id': {
                'type': 'string',
            },
            'instance_id': {
                'type': 'string',
            },
            'project': {  # DEPRECATED
                'type': 'string',
            },
            'folder_id': {  # Non-required in case of outdated extension version
                'type': 'string',
            },
            'quota': {  # Non-required in case of outdated extension version
                'type': 'string',
            },
        },
        'required': [
            'arcadia_path',
            'workflow_id',
            'instance_id',
        ],
        'additionalProperties': False,
    }


default_handlers = [
    ('/api/nirvana/check_oauth', NirvanaCheckOAuthHandler),
    ('/api/nirvana/workflow_info', NirvanaWorkflowInfo),
    ('/api/nirvana/new_jupyter_workflow', NirvanaNewJupyterWorkflow),
    ('/api/nirvana/add_to_draft', NirvanaAddToDraft),
    ('/api/nirvana/add_to_cloned_instance', NirvanaAddToClonedInstance),
    ('/api/nirvana/add_to_cloned_workflow', NirvanaAddToClonedWorkflow),
]

for (path, handler) in default_handlers[:]:
    default_handlers.append(
        ((path + '/' + NAME_RE), handler),
    )
