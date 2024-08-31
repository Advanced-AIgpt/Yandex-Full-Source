import asyncio
import logging
from datetime import datetime
from typing import Dict, Optional
from urllib.parse import urljoin

import aiohttp
from ci.tasklet.common.proto import service_pb2 as ci
from startrek_client import Startrek
from tasklet.services.ci import get_ci_env
from tasklet.services.yav.proto import yav_pb2 as yav

from alice.tasklet.create_release_ticket.proto import create_release_ticket_tasklet

ST_TOKEN_KEY = 'st.token'
ST_API_URL = 'https://st-api.yandex-team.ru'
ST_API_TEST_URL = 'https://st-api.test.yandex-team.ru'
ST_UI_URL = 'https://st.yandex-team.ru'
ST_UI_TEST_URL = 'https://st.test.yandex-team.ru'

WIKI_API_URL = 'https://wiki-api.yandex-team.ru/_api/frontend/'
WIKI_URL = 'https://wiki.yandex-team.ru/'

DEFAULT_SUMMARY_TEMPLATE = 'Релиз wsproxy, cuttlefish и графов apphost {}'
DEFAULT_RELEASE_CHAT = 'https://t.me/joinchat/CRF5o0R2B0P1wyfBKvK-Kw'

SANDBOX_RESOURCE_URL = 'https://sandbox.yandex-team.ru/resource/{}/view'

CHANGELOG_LIMIT = 300


class CreateReleaseTicketImpl(create_release_ticket_tasklet.CreateReleaseTicketBase):
    def run(self):
        st_token = self.ctx.yav.get_secret(
            yav.YavSecretSpec(uuid=self.input.context.secret_uid, key=ST_TOKEN_KEY),
            default_key=ST_TOKEN_KEY
        ).secret

        queue = self.input.config.queue
        self.major_version = int(self.input.context.version_info.major or "0")
        self.release_chat_url = DEFAULT_RELEASE_CHAT
        self.ticket_summary = DEFAULT_SUMMARY_TEMPLATE
        if self.input.config.release_chat_url is not None:
            self.release_chat_url = self.input.config.release_chat_url
        self.wiki_path = self.input.config.wiki_path
        if self.input.config.ticket_summary_template is not None:
            self.ticket_summary = self.input.config.ticket_summary_template
        if self.input.config.test_run is True:
            st_url = ST_API_TEST_URL
            st_ui_url = ST_UI_TEST_URL
        else:
            st_url = ST_API_URL
            st_ui_url = ST_UI_URL

        self.client = Startrek(
            useragent="Voicetech CI tasklet",
            base_url=st_url,
            token=st_token
        )

        ticket = self.check_for_existing_ticket(queue)
        if ticket is None:
            ticket = self.create_new_ticket(queue)

        progress_report = self._prepare_progress_update(st_ui_url, ticket)
        self.ctx.ci.UpdateProgress(progress_report)

        formatted_changelog = self._get_changelog()
        formatted_resources = self._get_resources()
        wiki_changelog_url = self._make_wiki_paths()
        self._changelog_to_wiki(st_token,
                                wiki_changelog_url['api'],
                                ticket,
                                formatted_changelog,
                                formatted_resources)
        self.fill_ticket_info(ticket, wiki_changelog_url['wiki'], formatted_changelog)

        self.output.state.success = True
        self.output.ticket.id = ticket

    def check_for_existing_ticket(self, queue: str) -> Optional[str]:
        logging.info("Searching for ticket")
        issues = self.client.issues.find(
            filter={'queue': queue,
                    'summary': self.ticket_summary.format(self.major_version)
                    }
        )
        if len(issues) == 0:
            return None
        return issues[0].key

    def create_new_ticket(self, queue: str) -> str:
        created_ticket = self.client.issues.create(
            queue=queue,
            summary=self.ticket_summary.format(self.major_version),
        )
        return created_ticket.key

    def fill_ticket_info(self, ticket: str, wiki_changelog_url: str, changelog: str):
        description = self._make_description(wiki_changelog_url, changelog)
        issue = self.client.issues[ticket]
        issue.update(description=description)

    def _make_description(self, wiki_changelog_url: str, changelog: str) -> str:
        description = \
            f'''[(({self.release_chat_url} Release chat))] [(({self.input.context.ci_job_url} Release url))] [(({wiki_changelog_url} Changelog))]
<{{**Добавленные коммиты**
{changelog}
}}>'''
        return description

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

    def _prepare_progress_update(self, st_url, ticket):
        startrek_url = urljoin(st_url, ticket)

        progress = ci.TaskletProgress()
        progress.job_instance_id.CopyFrom(self.input.context.job_instance_id)
        progress.id = 'ST url'
        progress.text = "Startrek ticket"
        progress.url = startrek_url
        progress.module = "STARTREK"
        progress.status = ci.TaskletProgress.Status.RUNNING
        return progress

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
