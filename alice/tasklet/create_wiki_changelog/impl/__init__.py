import asyncio
import logging
from datetime import datetime
from typing import Dict
from urllib.parse import urljoin

import aiohttp
from ci.tasklet.common.proto import service_pb2 as ci
from tasklet.services.ci import get_ci_env
from tasklet.services.yav.proto import yav_pb2 as yav

from alice.tasklet.create_wiki_changelog.proto import create_wiki_changelog_tasklet

ST_TOKEN_KEY = 'st.token'

WIKI_API_URL = 'https://wiki-api.yandex-team.ru/_api/frontend/'
WIKI_URL = 'https://wiki.yandex-team.ru/'

SANDBOX_RESOURCE_URL = 'https://sandbox.yandex-team.ru/resource/{}/view'

CHANGELOG_LIMIT = 300


class CreateWikiChangelogImpl(create_wiki_changelog_tasklet.CreateWikiChangelogBase):
    def run(self):
        st_token = self.ctx.yav.get_secret(
            yav.YavSecretSpec(uuid=self.input.context.secret_uid, key=ST_TOKEN_KEY),
            default_key=ST_TOKEN_KEY
        ).secret

        self.major_version = int(self.input.context.version_info.major or "0")
        self.wiki_path = self.input.config.wiki_path

        ticket = self.input.config.ticket
        formatted_changelog = self._get_changelog()
        formatted_resources = self._get_resources()
        wiki_changelog_url = self._make_wiki_paths()
        self._changelog_to_wiki(st_token,
                                wiki_changelog_url['api'],
                                ticket,
                                formatted_changelog,
                                formatted_resources)

        self.output.state.success = True

    def _changelog_to_wiki(self, token: str, wiki_url: str, ticket: str, changelog: str, resources: str):
        data = {'title': f'Changelog for {self.input.config.project} - {self.input.context.version_info.major}',
                'body': f'''Creation time: {datetime.now()}
Startrek: {ticket}
===Changelog:===
{changelog}
===Release items===
{resources}
'''}
        asyncio.run(wiki_request(wiki_url, data, token))

    def _get_changelog(self) -> str:
        changelog = '#|\n'
        request = ci.GetCommitsRequest()
        request.flow_launch_id = self.input.context.job_instance_id.flow_launch_id
        request.limit = CHANGELOG_LIMIT
        request.ci_env = get_ci_env(self.input.context)
        response = self.ctx.ci.GetCommits(request)
        for c in response.commits:
            changelog += self._format_commits(c)
        changelog += '|#\n'
        return changelog

    def _get_resources(self) -> str:
        resources = '#|\n'
        for resource in self.input.config.resources:
            resources += f'|| {resource.name} | (({SANDBOX_RESOURCE_URL.format(resource.resource_id)} {resource.resource_id})) || '
        resources += '|#\n'
        return resources

    @staticmethod
    def _format_commits(commit) -> str:
        if commit.revision.hash:
            revision = f'((https://a.yandex-team.ru/arc_vcs/commit/{commit.revision.hash} {commit.revision.hash[:10]}))'
        else:
            revision = f'((https://a.yandex-team.ru/arc/commit/{commit.revision.number} {commit.revision.number}))'
        message = commit.message.rsplit('\n')[0]
        return f'|| {revision} | {message} ||\n'

    def _make_wiki_paths(self) -> Dict[str, str]:
        path = self.wiki_path.rstrip('/') + f'/stable-{self.major_version}'
        return {'api': urljoin(WIKI_API_URL, path),
                'wiki': urljoin(WIKI_URL, path)}


async def wiki_request(url, data: Dict, token: str):
    async with aiohttp.ClientSession() as session:
        async with session.post(url,
                                headers={"Authorization": f"OAuth {token}"},
                                json=data) as resp:
            logging.info(await resp.text())
