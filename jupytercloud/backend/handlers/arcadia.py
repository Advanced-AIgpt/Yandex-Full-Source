import pathlib
import re

from tornado import web
from traitlets import Integer, Unicode
from traitlets.config import SingletonConfigurable

from jupytercloud.backend.handlers.base import JCAPIHandler, JCPageHandler
from jupytercloud.backend.handlers.utils import NAME_RE, PATH_RE, admin_or_self, require_server
from jupytercloud.backend.jupyticket import JupyTicket
from jupytercloud.backend.lib.util.paths import get_safe_path
from jupytercloud.backend.vcs.arc import Arc
from jupytercloud.backend.vcs.arcadia import (
    ArcadiaUploader, CommitSecretAttempt, FileIsTooBig, VCSError, WrongNodeType,
)

from .utils import json_body


MB = 1024 ** 3

FILENAME_RE = re.compile(r'(?:-(?P<nu>\d{1,4}))?\.ipynb$')


def canonize_path(path):
    path = pathlib.PurePath(path)
    if path.is_absolute():
        path = path.relative_to('/')

    return path


class ArcadiaHelper(SingletonConfigurable):
    """Class needed mostly for convenient configuring via traitlets config."""

    max_upload_file_size = Integer(5 * MB, config=True)
    max_download_file_size = Integer(20 * MB, config=True)
    base_url = Unicode('https://a.yandex-team.ru/arc/trunk/arcadia', config=True)

    def get_path_to_upload(self, username, path):
        raw_path = pathlib.PurePath('junk') / username / 'jupyter' / path

        return get_safe_path(raw_path)

    def get_path_to_download(self, path):
        return pathlib.PurePath('ArcadiaDownloads') / path.name

    def get_arcadia_link(self, path, revision=None):
        link = f'{self.base_url}/{path}'
        if revision:
            link = f'{link}?rev={revision}'
        return link


class ArcadiaHandlerMixin:
    _arcadia = None
    _arc = None

    @property
    def arcadia(self):
        if self._arcadia is None:
            self._arcadia = ArcadiaHelper.instance(config=self.settings['config'])

        return self._arcadia

    @property
    def arc(self):
        if self._arc is None:
            self._arc = Arc.instance(
                log=self.log,
                config=self.settings['config'],
            )

        return self._arc

    async def _get_arcadia_file_content(self, path, rev):
        path = canonize_path(path)

        try:
            return (
                await self.arc.read_path(
                    path,
                    rev,
                    max_size=self.arcadia.max_download_file_size,
                )
            ).decode('utf-8')
        except (FileIsTooBig, WrongNodeType) as e:
            raise web.HTTPError(400, str(e)) from e
        except VCSError as e:
            raise web.HTTPError(500, str(e)) from e


class BaseArcadiaHandler(JCAPIHandler, ArcadiaHandlerMixin):
    pass


class SaveToArcadiaHandler(BaseArcadiaHandler):
    post_schema = {
        'type': 'object',
        'properties': {
            'jupyticket_id': {'type': 'integer'},
            'message': {'type': 'string'},
            'title': {'type': 'string'},
            'startrek_tickets': {
                'type': 'array',
                'items': {
                    'type': 'string',
                },
                'minItems': 0,
                'uniqueItems': True,
            },
            # XXX: path or arcadia_path is required, but it is too difficult to achieve
            # via json_schema validation
            'path': {'type': 'string'},
            'arcadia_path': {'type': 'string'},
        },
    }

    _uploader = None

    @property
    def uploader(self):
        if self._uploader is None:
            self._uploader = ArcadiaUploader(
                log=self.log,
                config=self.settings['config'],
            )

        return self._uploader

    async def validate_file_info(self, file_info):
        for value_name in ('type', 'size'):
            value = file_info.get(value_name)
            if value is None:
                return {
                    'result': False,
                    'reason': f'missing {value_name} parameter from jupyter file info',
                }

        if file_info['type'] != 'notebook':
            return {
                'result': False,
                'reason': 'only notebook files is allowed to upload',
            }

        if file_info['size'] > self.arcadia.max_upload_file_size:
            return {
                'result': False,
                'reason': 'file is too big {:.2f} > {:.2f}'.format(
                    file_info['size'] / MB, self.arcadia.max_upload_file_size / MB,
                ),
            }

        return {
            'result': True,
        }

    async def _generate_path_choices(self, username, path):
        match = FILENAME_RE.search(path.name)
        groups = match.groupdict()
        nu = int(groups['nu'] or 0)
        old_suffix = match.group(0)
        base_name = path.name[:-len(old_suffix)]

        for i in range(nu + 1, 10):
            new_suffix = f'-{i}.ipynb'

            suggested_path = path.with_name(base_name + new_suffix)
            path_to_upload = self.arcadia.get_path_to_upload(username, suggested_path)

            if not await self.uploader.check_path_exists(path_to_upload):
                return [str(suggested_path)]

        return []

    @web.authenticated
    @admin_or_self
    @require_server
    async def get(self, name, path):
        username = name  # to reuse @admin_or_self

        user = self.user_from_username(username)
        path = canonize_path(path)
        path_to_upload = self.arcadia.get_path_to_upload(username, path)

        exists = await self.uploader.check_path_exists(path_to_upload)

        try:
            await self.uploader.assert_can_upload(path_to_upload)
        except WrongNodeType as e:
            self.write({
                'result': False,
                'exists': exists,
                'message': str(e),
                'link': self.arcadia.get_arcadia_link(path_to_upload),
            })
            return

        with self.jupyter_client(user) as client:
            file_info = await client.get_file_info(path)
            if file_info is None:
                raise web.HTTPError(404, f'no such file or directory: {path}')

        upload_possibility = await self.validate_file_info(file_info)
        if not upload_possibility['result']:
            self.write({
                'result': False,
                'exists': exists,
                'message': upload_possibility['reason'],
                'link': self.arcadia.get_arcadia_link(path_to_upload),
            })
            return

        if exists:
            name_choices = await self._generate_path_choices(username, path)
            upload_path_choices = [
                str(self.arcadia.get_path_to_upload(username, p))
                for p in name_choices
            ]
        else:
            name_choices = []
            upload_path_choices = []

        self.write({
            'result': True,
            'exists': exists,
            'message': 'upload is possible',
            'default_path': str(path_to_upload),
            'additional_choices': name_choices,
            'upload_path_choices': upload_path_choices,
            'link': self.arcadia.get_arcadia_link(path_to_upload),
        })

    @web.authenticated
    @admin_or_self
    @require_server
    @json_body
    async def post(self, name, path, json_data):
        username = name  # to reuse @admin_or_self

        user = self.user_from_username(username)

        path = canonize_path(path)

        # path is old parameter, which represents filepath on user's server
        if 'path' in json_data:
            raw_path_to_upload = canonize_path(json_data['path'])
            path_to_upload = self.arcadia.get_path_to_upload(username, raw_path_to_upload)
        # arcadia_path is new parameter, which represents arcadia path already
        elif 'arcadia_path' in json_data:
            path_to_upload = pathlib.PurePath(json_data['arcadia_path'])
        else:
            raise web.HTTPError(400, 'either path or arcadia_path must be in json body')

        jupyticket = None
        jupyticket_kwargs = dict(
            db=self.jupyter_cloud_db,
            startrek_client=self.startrek_client,
        )
        if jupyticket_id := json_data.get('jupyticket_id'):
            jupyticket = JupyTicket.get(
                id=jupyticket_id,
                **jupyticket_kwargs,
            )
            if not jupyticket:
                raise web.HTTPError(404, f'jupyticket with id {jupyticket_id} does not exists')

        # TODO: Validate tickets here
        tickets = json_data.get('startrek_tickets', [])

        description = f'Shared by {username}@ via {self.jupyter_public_host}'
        if raw_message := json_data.get('message', '').strip():
            description = f'{raw_message}\n{description}'

        if title := json_data.get('title', '').strip():
            message = f'{title}\n\n{description}'
        else:
            message = description

        with self.jupyter_client(user) as client:
            file_info = await client.get_file_info(path)
            if file_info is None:
                raise web.HTTPError(404, f'no such file or directory: {path}')

            upload_possibility = await self.validate_file_info(file_info)
            if not upload_possibility['result']:
                raise web.HTTPError(400, upload_possibility['reason'])

            file_info = await client.get_content(path)
            content = file_info['content']

        try:
            revision = await self.uploader.upload_file(
                path=path_to_upload,
                content=content,
                commit_message=message,
            )
        except CommitSecretAttempt as e:
            raise web.HTTPError(
                400,
                'Secrets found in notebook.\n'
                'Saving secrets to repository is forbidden.\n'
                'Please remove all secrets and try again.',
                reason='Secrets in Notebook',
            ) from e
        except WrongNodeType as e:
            raise web.HTTPError(400, str(e)) from e
        except VCSError as e:
            raise web.HTTPError(500, str(e)) from e

        arcadia_path = str(path_to_upload)
        title = title or f'Share to {path_to_upload}'
        revision = revision or await self.uploader.get_file_revision(arcadia_path)

        # jupyticket_id is not provided, so we creating new one
        if not jupyticket:
            # we need to create jupyticket outside transaction to create and operate
            # with new autoincrement id
            jupyticket = JupyTicket.new(
                user_name=username,
                title=title,
                description=description,
                **jupyticket_kwargs,
            )

        with self.jupyter_cloud_db.transaction():
            jupyticket.add_new_arcadia_share(
                user_name=username,
                path=arcadia_path,
                revision=revision,
                message=message,
            )

            if tickets:
                await jupyticket.set_startrek_tickets(tickets)

        self.write({
            'message': 'file succesfully uploaded',
            'link': self.arcadia.get_arcadia_link(path_to_upload, revision),
            'path': arcadia_path,
            'revision': revision,
            'jupyticket_id': jupyticket.id,
        })


class DownloadFromArcadiaHandler(BaseArcadiaHandler):
    @admin_or_self
    @require_server
    async def post(self, name, path):
        user = self.user_from_username(name)
        body = self.get_json_body()
        rev = body.get('rev')

        if not rev:
            raise web.HTTPError(400, 'missing rev parameter from json body')

        target_path = body.get('path', path)
        target_path = canonize_path(target_path)

        self.log.info(
            "downloading file '%s:%s' from arcadia and uploading it to %s's server as %s",
            rev, path, name, target_path,
        )

        content = await self._get_arcadia_file_content(path, rev)

        with self.jupyter_client(user) as client:
            await self._write_file(client, target_path, content)

        self.write({'result': True})


class DownloadFromArcadiaRedirectHandler(JCPageHandler, ArcadiaHandlerMixin):
    @web.authenticated
    @admin_or_self
    async def get(self, name, path):
        user = self.user_from_username(name)
        rev = self.get_argument('rev')
        path = canonize_path(path)

        if user.spawner.active:
            content = await self._get_arcadia_file_content(path, rev)
            raw_target_path = self.arcadia.get_path_to_download(path)
            target_path = await self.write_file_for_user(user, raw_target_path, content)

            next_url = self.get_next_url_notebook(user, target_path)
        else:
            next_url = self.get_next_url_through_spawn(
                name, f'/arcadia/download/{name}/{path}?rev={rev}',
            )

        self.redirect(next_url)


class AnonymousDownloadFromArcadiaHandler(JCPageHandler):
    @web.authenticated
    async def get(self, path):
        if self.current_user is None:
            raise web.HTTPError(403)

        rev = self.get_argument('rev')
        next_url = self.get_next_url(
            default=f'/arcadia/download/{self.current_user.name}{path}?rev={rev}',
        )
        self.redirect(next_url)


default_handlers = [
    (
        f'/api/arcadia/upload/{NAME_RE}{PATH_RE}',
        SaveToArcadiaHandler,
    ),
    (
        f'/api/arcadia/download/{NAME_RE}{PATH_RE}',
        DownloadFromArcadiaHandler,
    ),
    (
        f'/arcadia/download/{NAME_RE}{PATH_RE}',
        DownloadFromArcadiaRedirectHandler,
    ),
    (
        f'/redirect/arcadia/download{PATH_RE}',
        AnonymousDownloadFromArcadiaHandler,
    ),
]
