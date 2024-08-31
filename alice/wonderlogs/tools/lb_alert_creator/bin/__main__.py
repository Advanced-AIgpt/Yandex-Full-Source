import json

import click
from alice.wonderlogs.tools.lb_alert_creator.lib.alerts import generate_all_alerts
from alice.wonderlogs.tools.lb_alert_creator.lib.config import Config


@click.command()
@click.option('--config-path', required=True, type=click.File())
def main(config_path):
    config = Config.from_dict(json.load(config_path))
    alerts = [a.to_solomon_dict() for a in generate_all_alerts(config)]
    print(json.dumps(alerts, indent=4, sort_keys=True))


if __name__ == "__main__":
    main()
