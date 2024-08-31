import asyncio
import json
import pathlib
import re
import tempfile

from traitlets import Instance, Integer, List, Unicode, default
from traitlets.config import LoggingConfigurable, SingletonConfigurable

from jupytercloud.backend.lib.util.exc import JupyterCloudException
from jupytercloud.backend.lib.util.logging import LoggingContextMixin


class VCSError(JupyterCloudException):
    pass


class FileIsTooBig(JupyterCloudException):
    pass


class WrongNodeType(VCSError):
    pass


class CommitError(VCSError):
    pass


class CommitSecretAttempt(CommitError):
    def __init__(self, error_text):
        self.error_text = self._parse(error_text)

    def _parse(self, error_text):
        result = []
        lines = error_text.split('\n')

        for line in lines:
            line = line.strip()

            if not line or line == 'Changeset contains secret data':
                continue

            if result and not line.startswith('>'):
                result[-1] += ' ' + line
            else:
                line = ' >' + line.lstrip('>')
                result.append(line)

        return '\n'.join(result)

    def __str__(self):
        return self.error_text


class BaseVCS(LoggingContextMixin):
    base_command = List(Unicode())

    @property
    def log_context(self):
        return None

    async def _create_process(self, *args, env=None, cwd=None):
        env = env or {}
        env.setdefault('LC_ALL', 'en_US.UTF-8')

        command = list(self.base_command)
        command.extend(
            str(arg) if isinstance(arg, pathlib.PurePath) else arg
            for arg in args
        )

        self.log.debug('executing command %s', command)

        return await asyncio.create_subprocess_exec(
            *command,
            env=env,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE,
            cwd=cwd,
        )

    async def _execute(self, *args, raise_error=True, env=None):
        proc = await self._create_process(*args, env=env)

        stdout, stderr = await proc.communicate()
        returncode = proc.returncode

        if returncode and raise_error:
            raise VCSError(
                f'command {self.base_command} with args {args} have non-zero returncode {returncode}; '
                f'stdout: {stdout}; '
                f'stderr: {stderr}; ',
            )

        return dict(
            stdout=stdout.decode('utf-8').strip(),
            stderr=stderr.decode('utf-8').strip(),
            returncode=returncode,
        )


class SVN(SingletonConfigurable, BaseVCS):
    proto = Unicode('svn+ssh')
    base_url = Unicode('arcadia.yandex.ru/arc/trunk/arcadia', config=True)

    base_command = List(Unicode(), ['/usr/bin/svn'])
    ssh_key_path = Unicode(config=True)
    ssh_user = Unicode(config=True)

    revision_re = re.compile(r'^Committed revision (\d+).$', re.MULTILINE)

    @property
    def log_context(self):
        return None

    def _get_url(self, svn_path):
        return '{proto}://{user}@{base_url}/{path}'.format(
            proto=self.proto,
            user=self.ssh_user,
            base_url=self.base_url.rstrip('/'),
            path=str(svn_path).lstrip('/'),
        )

    async def _execute(self, *args, raise_error=True, env=None):
        assert self.ssh_key_path
        env = env or {}
        env['SVN_SSH'] = (
            f'ssh -i {self.ssh_key_path} '
            '-o StrictHostKeyChecking=no '
            '-o UserKnownHostsFile=/dev/null '
        )

        return await super()._execute(*args, raise_error=raise_error, env=env)

    async def check_path_exists(self, svn_path):
        res = await self._execute(
            'ls', self._get_url(svn_path), '--depth', 'empty', raise_error=False,
        )

        return res['returncode'] == 0

    async def get_file_revision(self, svn_path):
        res = await self._execute(
            'info', self._get_url(svn_path),
            '--show-item', 'last-changed-revision',
        )

        return res['stdout']

    async def find_existing_path_part(self, svn_path):
        svn_path = pathlib.PurePath(svn_path)

        if await self.check_path_exists(svn_path):
            return svn_path

        for parent in svn_path.parents:
            if await self.check_path_exists(parent):
                return parent

        return None

    async def checkout(self, svn_path, local_path):
        await self._execute(
            'checkout', self._get_url(svn_path),
            '--depth', 'empty', '-q', local_path,
        )

    async def mkdir(self, local_path):
        await self._execute('mkdir', '--parents', '-q', local_path)

    async def add(self, local_path):
        await self._execute('add', '-q', local_path)

    async def commit(self, local_path, message):
        result = await self._execute(
            'commit', '--with-revprop', 'arcanum:force=yes',
            '--message', message, local_path,
            raise_error=False,
        )
        if result['returncode']:
            parts = result['stderr'].split('Secrets:\n', 1)
            if len(parts) > 1:
                raise CommitSecretAttempt(parts[1])

            raise CommitError(result['stderr'])

        revision_match = self.revision_re.search(result['stdout'])

        if not revision_match:
            return None

        return revision_match.group(1)

    async def _info(self, svn_path):
        result = await self._execute('info', self._get_url(svn_path), raise_error=False)

        if result['returncode']:
            if 'W170000' in result['stderr']:
                return None
            raise VCSError(str(result))

        info = {}

        for line in result['stdout'].splitlines():
            line = line.strip()
            if not line:
                continue

            k, v = line.split(':', 1)
            info[k] = v.lstrip()

        return info

    async def get_node_type(self, svn_path):
        info = await self._info(svn_path)
        if not info:
            return None

        type_ = info['Node Kind']

        if type_ in {'directory', 'file'}:
            return type_

        raise VCSError(f'unknown type of node {svn_path}: {type_}')

    async def update(self, local_path):
        await self._execute('update', '-q', local_path)


class ArcadiaUploader(LoggingConfigurable, LoggingContextMixin):
    upload_timeout = Integer(10 * 60, config=True)
    temp_dir_prefix = Unicode('arcadia-upload-', config=True)

    svn = Instance(SVN)

    @property
    def log_context(self):
        return None

    @default('svn')
    def _svn_default(self):
        return SVN.instance(
            log=self.log,
            config=self.config,
        )

    async def check_path_exists(self, path):
        return await self.svn.check_path_exists(path)

    async def get_file_revision(self, path):
        return await self.svn.get_file_revision(path)

    async def assert_can_upload(self, path):
        node_type = await self.svn.get_node_type(path)
        if node_type and node_type != 'file':
            raise WrongNodeType(
                f"can't upload file to {path} because it exists and it is not a file",
            )

        for sub_path in path.parents:
            node_type = await self.svn.get_node_type(sub_path)

            if not node_type:
                continue

            if node_type == 'directory':
                return

            raise WrongNodeType(
                f"can't upload file to {path} because {sub_path} exists and it is not a dir",
            )

    async def upload_file(self, path, content, commit_message):
        self.log.info('uploading file to %s', path)
        path = pathlib.PurePath(path)
        await self.assert_can_upload(path)

        file_exists = await self.svn.check_path_exists(path)
        if file_exists:
            existing_dir = path.parent
        else:
            existing_dir = await self.svn.find_existing_path_part(path.parent)

        with tempfile.TemporaryDirectory(prefix=self.temp_dir_prefix) as temp_dir:
            await self.svn.checkout(
                svn_path=existing_dir,
                local_path=temp_dir,
            )

            path_to_create = path.parent.relative_to(existing_dir)
            local_dir = temp_dir / path_to_create

            # A.relative_to(B) == Path('.') <=> A == B & Path('.').name == ''
            if path_to_create.name:
                await self.svn.mkdir(local_dir)

            local_file = local_dir / path.name

            if file_exists:
                await self.svn.update(local_file)

            with open(local_file, 'w') as f_:
                json.dump(content, f_, indent=2)

            if not file_exists:
                await self.svn.add(local_file)

            return await self.svn.commit(temp_dir, commit_message)
