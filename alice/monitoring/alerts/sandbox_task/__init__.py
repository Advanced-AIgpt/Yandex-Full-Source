import json
import logging
import os

from alice.monitoring.alerts.solomon_syncer.lib.sync import sync_alerts
from sandbox import sdk2
from sandbox.common.errors import TaskError
from sandbox.projects.common.vcs import arc


class SyncSolomonAlerts(sdk2.Task):
    class Parameters(sdk2.Task.Parameters):

        alerts_dir = sdk2.parameters.String('Directory in Arcadia with alerts',
                                            default='alice/monitoring/alerts/alerts_data')
        solomon_project = sdk2.parameters.String(
            'Solomon project e.g. megamind from https://solomon.yandex-team.ru/admin/projects/megamind',
            default='megamind')

        tokens = sdk2.parameters.YavSecret('Solomon token', required=True)

    def on_execute(self):
        arc_client = arc.Arc()
        alerts = []
        with arc_client.mount_path(None, None, fetch_all=False) as trunk:
            logging.info('Scanning files:')
            for root, dirs, files in os.walk(os.path.join(trunk, self.Parameters.alerts_dir)):
                for name in files:
                    if not name.endswith('.json'):
                        continue
                    file = os.path.join(root, name)
                    logging.info(file)
                    with open(file, 'r') as f:
                        alerts.extend(json.load(f))
            logging.info('Scanning finished.')
        flag = True
        report = 'OK'
        logging.debug(alerts)
        try:
            sync_alerts(self.Parameters.tokens.data()[self.Parameters.tokens.default_key],
                        self.Parameters.solomon_project, alerts)
        except Exception as exc:
            logging.error(exc)
            flag = False
            report = str(exc)

        self.Context.report = report

        if not flag:
            raise TaskError(f'Failed sync alerts:\n{report}')

    @sdk2.footer()
    def report(self):
        return self.Context.report
