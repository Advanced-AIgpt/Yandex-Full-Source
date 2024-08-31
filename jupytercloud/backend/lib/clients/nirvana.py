import asyncio
import functools
import re
from concurrent.futures import Executor, ThreadPoolExecutor

import nirvana_api
import nirvana_api.api
import nirvana_api.highlevel_api
from nirvana_api.blocks import BaseBlock, ProcessorType
from nirvana_api.blocks.svn_blocks import SvnExportSingleFile
from traitlets import Instance, Unicode, default
from traitlets.config import LoggingConfigurable

from jupytercloud.backend.lib.util.logging import LoggingContextMixin


JUPYTER_BLOCK_DESCRIPTION = {
    'parameters': [
        'verify-kernel-specification',
        'kernel-revision',
        'notebook-parameters',
        'yav-oauth-token',
    ],
    'input_names': [
        'notebook',
        'parameters_json',
        'data',
    ],
    'output_names': [
        'notebook',
        'notebook_html',
        'notebook_stdout',
        'custom_output_0',
        'custom_output_1',
        'custom_output_2',
        'custom_output_3',
        'custom_output_4',
    ],
    'processor_type': ProcessorType.Job,
}

JUPYTER_CODE_PREFIX = 'run-jupyter-notebook'
EXPORT_SINGLE_FILE_CODE_PREFIX = 'run-jupyter-notebook-export'


def generate_block_code(prefix, codes=None):
    codes = codes or []

    numbers = []

    for code in codes:
        number_match = re.match(prefix + r'-(\d+)$', code)
        if number_match:
            numbers.append(int(number_match.group(1)))

    new_number = 0
    for number in numbers:
        if new_number != number:
            break

        new_number += 1

    return f'{prefix}-{new_number}'


class AsyncWrapper:
    def __init__(self, client, target):
        self._client = client
        self._target = target

    def __getattr__(self, name):
        obj = getattr(self._target, name)

        if callable(obj):
            @functools.wraps(obj)
            async def wrapper(*args, **kwargs):
                return await self._client.call_method(obj, *args, **kwargs)

            return wrapper

        return obj


class NirvanaClient(LoggingConfigurable, LoggingContextMixin):
    oauth_token = Unicode()
    username = Unicode()
    server = Unicode(nirvana_api.api.NIRVANA_HOST, config=True)

    client = Instance(nirvana_api.NirvanaApi)
    executor = Instance(Executor)
    loop = Instance(asyncio.AbstractEventLoop)

    jupyter_oauth_token = Unicode(config=True)
    jupyter_user = Unicode(config=True)
    jupyter_block_alias = Unicode('run-jupyter-notebook', config=True)
    jupyter_default_workflow = Unicode('jupytercloud_junk', config=True)
    jupyter_block_code_prefix = Unicode('run-jupyter-notebook-', config=True)

    @classmethod
    def get_jupyter_client(cls, **kwargs):
        result = cls(**kwargs)
        result.username = result.jupyter_user
        result.oauth_token = result.jupyter_oauth_token
        return result

    @property
    def log_context(self):
        return {}

    @default('client')
    def _client_default(self):
        return nirvana_api.NirvanaApi(
            oauth_token=self.oauth_token,
            server=self.server,
        )

    @default('executor')
    def _executor_default(self):
        return ThreadPoolExecutor()

    @default('loop')
    def _loop_default(self):
        return asyncio.get_running_loop()

    async def call_method(self, method, *args, **kwargs):
        @functools.wraps(method)
        def wrapper():
            return method(*args, **kwargs)

        return await self.loop.run_in_executor(
            self.executor,
            wrapper,
        )

    @functools.lru_cache(None)
    def __getattr__(self, name):
        # without this `if` all goes wrong:
        # HasTraits.__new__ tries to find Nirvana.client._something_validate method
        # and this leads to invoking NirvanaClient._client_default before
        # setting up NirvanaClient.oauth_token trait
        if name.startswith('_'):
            return super().__getattribute__(name)

        obj = getattr(self.client, name)

        if callable(obj):
            @functools.wraps(obj)
            async def wrapper(*args, **kwargs):
                return await self.call_method(obj, *args, **kwargs)

            return wrapper

        return obj

    def async_wrap(self, target):
        return AsyncWrapper(self, target)

    @functools.cached_property
    def highlevel_api(self):
        return self.async_wrap(nirvana_api.highlevel_api)

    async def get_jupyter_block_class(self):
        block_info = await self.get_operation_by_alias(self.jupyter_block_alias)

        self.log.debug(
            'fetched info about main jupyter block with version %s and id %s',
            block_info.versionNumber,
            block_info.operationId,
        )

        block_dict = JUPYTER_BLOCK_DESCRIPTION.copy()
        block_dict.update({
            'guid': block_info.operationId,
            'name': block_info.name,
        })

        return type('DynamicJupyterBlock', (BaseBlock,), block_dict)


async def create_jupyter_workflow(
    nirvana,
    *,
    arcadia_path,
    description,
    kernel_revision=None,
    project=None,  # DEPRECATED
    folder_id=None,  # None by-default in case of outdated extension version
    arcadia_revision=None,  # None by-default in case of outdated extension version
    quota=None,  # None by-default in case of outdated extension version
):
    svn_path = f'arcadia/{arcadia_path}'
    svn_export_single_file = SvnExportSingleFile(
        arcadia_path=svn_path,
        revision=arcadia_revision,
        code=generate_block_code(EXPORT_SINGLE_FILE_CODE_PREFIX),
    )

    DynamicJupyterBlock = await nirvana.get_jupyter_block_class()

    jupyter_block = DynamicJupyterBlock(
        notebook=svn_export_single_file.outputs.json,
        kernel_revision=kernel_revision,
        code=generate_block_code(JUPYTER_CODE_PREFIX),
    )

    nirvana.log.debug(
        'creating workflow with blocks: %s',
        [block.guid for block in (svn_export_single_file, jupyter_block)],
    )

    workflow_name = 'JupyterCloud Workflow {}'.format(
        arcadia_path.replace('/', '\\')
    )
    workflow = await nirvana.highlevel_api.create_workflow(
        nirvana.client,
        workflow_name,
        [jupyter_block],
        quota_project_id=quota,
        ns_id=folder_id,
        description=description,
    )
    async_workflow = nirvana.async_wrap(workflow)

    url = await async_workflow.get_url()

    nirvana.log.info('created workflow %s with url %s', workflow.id, url)

    instance = await async_workflow.get_instance()
    async_instance = nirvana.async_wrap(instance)

    await async_instance.layout()

    nirvana.log.debug(
        'auto-layout done for workflow %s and instance %s',
        workflow.id, instance.id,
    )

    return {
        'url': url,
        'workflow_id': workflow.id,
        'instance_id': instance.id,
    }


async def add_to_draft(
    nirvana,
    *,
    arcadia_path,
    workflow_id,
    instance_id,
    kernel_revision=None,
    arcadia_revision=None,  # None by-default in case of outdated extension version
):
    workflow_info = await nirvana.get_workflow(
        workflow_id=workflow_id,
        workflow_instance_id=instance_id,
    )

    block_codes = [block.blockCode for block in workflow_info.blocks]
    jupyter_block_code = generate_block_code(JUPYTER_CODE_PREFIX, codes=block_codes)
    export_block_code = generate_block_code(EXPORT_SINGLE_FILE_CODE_PREFIX, codes=block_codes)

    svn_path = f'arcadia/{arcadia_path}'
    svn_export_single_file = SvnExportSingleFile(
        arcadia_path=svn_path,
        revision=arcadia_revision,
        code=export_block_code,
    )

    DynamicJupyterBlock = await nirvana.get_jupyter_block_class()

    jupyter_block = DynamicJupyterBlock(
        notebook=svn_export_single_file.outputs.json,
        kernel_revision=kernel_revision,
        code=jupyter_block_code,
    )

    workflow = nirvana_api.Workflow(nirvana.client, workflow_id)

    nirvana.log.debug(
        'adding to workflow_id=%s and instance_id=%s blocks: %s',
        workflow_id,
        instance_id,
        [block.guid for block in (svn_export_single_file, jupyter_block)],
    )

    await nirvana.call_method(
        _add_nodes_to_instance,
        workflow=workflow,
        nodes=[jupyter_block, svn_export_single_file],
        workflow_instance_id=instance_id,
    )

    nirvana.log.debug(
        'blocks succesfully added to workflow_id=%s and instance_id=%s',
        workflow_id,
        instance_id,
    )

    async_workflow = nirvana.async_wrap(workflow)
    url = await async_workflow.get_url()

    await nirvana.layout_workflow_instance(workflow_instance_id=instance_id)

    nirvana.log.debug(
        'auto-layout done for workflow %s and instance %s',
        workflow_id, instance_id,
    )

    return {
        'url': url,
        'workflow_id': workflow_id,
        'instance_id': instance_id,
    }


async def add_to_cloned_instance(
    nirvana,
    *,
    arcadia_path,
    workflow_id,
    instance_id,
    revision=None,
    kernel_revision=None,
    arcadia_revision=None,  # None by-default in case of outdated extension version
):
    new_instance_id = await nirvana.clone_workflow_instance(
        workflow_instance_id=instance_id,
    )

    return await add_to_draft(
        nirvana,
        arcadia_path=arcadia_path,
        workflow_id=workflow_id,
        instance_id=new_instance_id,
        arcadia_revision=arcadia_revision,
        kernel_revision=kernel_revision,
    )


async def add_to_cloned_workflow(
    nirvana,
    *,
    arcadia_path,
    description,
    workflow_id,
    instance_id,
    revision=None,
    kernel_revision=None,
    project=None,  # DEPRECATED,
    folder_id=None,  # None by-default in case of outdated extension version
    arcadia_revision=None,  # None by-default in case of outdated extension version
    quota=None,  # None by-default in case of outdated extension version
):
    new_workflow_id = await nirvana.create_workflow(
        name=f'JupyterCloud Workflow {arcadia_path}',
        description=description,
        quota_project_id=quota,
        ns_id=folder_id,
    )

    new_instance_id = await nirvana.clone_workflow_instance(
        workflow_instance_id=instance_id,
        target_workflow_id=new_workflow_id,
    )

    return await add_to_draft(
        nirvana,
        arcadia_path=arcadia_path,
        workflow_id=new_workflow_id,
        instance_id=new_instance_id,
        arcadia_revision=arcadia_revision,
        kernel_revision=kernel_revision,
    )


def _add_nodes_to_instance(workflow, nodes, workflow_instance_id, follow_outputs=True):
    # XXX Этот ад в оригинальном АПИ абсолютно не изменяем.
    # Туда почти нереально добавить параметризацию от workflow_instance_id.

    from nirvana_api.highlevel_api import (
        BlockPattern, OperationBlock, _get_connections, _get_parameters, _split_nodes,
        _split_parameters,
    )
    data_nodes, operation_nodes = _split_nodes(nodes)

    workflow.add_operation_blocks(
        [OperationBlock(node.guid, node.name, node.code) for node in operation_nodes],
        workflow_instance_id=workflow_instance_id,
    )

    block_params = []
    references = []
    for node in operation_nodes:
        params, _ = _get_parameters(node)
        values, refs = _split_parameters(params)
        if values:
            block_params.append((BlockPattern(code=node.code), values))
        if refs:
            references.append((node, refs))
    with workflow.nirvana.batch():
        for param in block_params:
            workflow.set_block_parameters(
                *param,
                workflow_instance_id=workflow_instance_id,
            )
    with workflow.nirvana.batch():
        for ref in references:
            workflow.parameters.set_references(
                *ref,
                workflow_instance_id=workflow_instance_id,
            )

    data_connections, operation_connections, execution_connections = _get_connections(
        nodes, follow_outputs,
    )
    with workflow.nirvana.batch():
        for connection in data_connections:
            workflow.connect_data_blocks(
                *connection,
                workflow_instance_id=workflow_instance_id,
            )
    with workflow.nirvana.batch():
        for connection in operation_connections:
            workflow.connect_operation_blocks(
                *connection,
                workflow_instance_id=workflow_instance_id,
            )
    with workflow.nirvana.batch():
        for connection in execution_connections:
            workflow.connect_operation_blocks_by_execution_order(
                *connection,
                workflow_instance_id=workflow_instance_id,
            )
