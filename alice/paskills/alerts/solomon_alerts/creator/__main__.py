from alice.paskills.alerts.solomon_alerts.registry import objects_registry
from library.python.monitoring.solo.helpers.cli import build_basic_cli_v2

if __name__ == "__main__":
    cli = build_basic_cli_v2(
        objects_registry=objects_registry,
        juggler_mark="solo_paskills_juggler"
    )
    cli()
