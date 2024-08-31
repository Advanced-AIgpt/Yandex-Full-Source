import logging
from urllib.parse import urljoin

from ci.tasklet.common.proto import service_pb2 as ci
from startrek_client import Startrek
from tasklet.services.yav.proto import yav_pb2 as yav

from alice.tasklet.find_st_ticket.proto import find_st_ticket_tasklet

ST_TOKEN_KEY = 'st.token'
ST_API_URL = 'https://st-api.yandex-team.ru'
ST_UI_URL = 'https://st.yandex-team.ru'


class FindStTicketImpl(find_st_ticket_tasklet.FindStTicketBase):
    def run(self):
        st_token = self.ctx.yav.get_secret(
            yav.YavSecretSpec(uuid=self.input.context.secret_uid, key=ST_TOKEN_KEY),
            default_key=ST_TOKEN_KEY
        ).secret

        startrek_client = Startrek(
            useragent='Voicetech CI tasklet',
            base_url=ST_API_URL,
            token=st_token
        )

        template = self.input.config.ticket_summary_template
        summary = template.format(
            self.input.config.search_key
        )
        logging.info("Looking up tickets in %s that match %s", self.input.config.queue, summary)
        issues = startrek_client.issues.find(
            filter={
                'queue': self.input.config.queue,
                'summary': summary
            }
        )
        logging.info("Startrek response: %s", issues)
        if len(issues) > 0:
            result = issues[0].key
            progress_report = self._prepare_progress_update(ST_UI_URL, result)
            self.ctx.ci.UpdateProgress(progress_report)
        else:
            result = "not_found"
        logging.info(result)

        self.output.state.ticket_id = result
        self.output.state.success = True

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
