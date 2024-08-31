import json
import logging

import click
from alice.monitoring.alerts.solomon_syncer.lib.sync import sync_alerts


@click.command()
@click.option('--token', required=True)
@click.option('--project', required=True)
@click.option('--alerts-file', required=True, type=click.File())
def main(token, project, alerts_file):
    logging.basicConfig(level=logging.INFO)
    alerts = json.load(alerts_file)
    sync_alerts(token, project, alerts)


if __name__ == '__main__':
    main()
