import datetime

from juggler_sdk import Check, FlapOptions
from library.python.monitoring.solo.objects.solomon.v2 import Type, Threshold, Alert, PredicateRule
from alice.paskills.alerts.solomon_alerts.registry.channel.channels import memento_juggler
from alice.paskills.alerts.solomon_alerts.registry.project.projects import memento
from alice.paskills.alerts.solomon_alerts.registry.sensor.memento_sensors import get_all_objects_errors
from alice.paskills.alerts.solomon_alerts.registry.shard.shards import memento_prod

memento_get_all_objects_errors_alert = Alert(
    id='memento_get_all_objects_errors',
    name='get_all_objects errors',
    project_id=memento.id,
    type=Type(
        threshold=Threshold(
            selectors=get_all_objects_errors.selectors,
            predicate_rules=[
                PredicateRule(
                    threshold_type="MAX",
                    comparison="GT",
                    threshold=5,
                    target_status="WARN"
                ),
                PredicateRule(
                    threshold_type="MAX",
                    comparison="GT",
                    threshold=15,
                    target_status="ALARM"
                ),
            ]
        )
    ),
    delay_seconds=0,
    annotations={
        "graphLink": "https://solomon.yandex-team.ru/?project=memento&cluster=production&service="
        "main_monitoring&l.sensor=http.in.requests_failure_rate&l.path=get_all_objects&l.host=cluster&graph=auto",
        'service': 'memento_production',
        'host': memento_prod.id
    },
    window_secs=int(datetime.timedelta(seconds=30).total_seconds()),
    notification_channels={memento_juggler.id}
)

memento_get_all_objects_errors_check = Check(
    host=memento_get_all_objects_errors_alert.annotations["host"],
    service=memento_get_all_objects_errors_alert.annotations["service"],
    namespace="paskills",
    refresh_time=60, ttl=600,
    aggregator="logic_or",
    tags={'telegram'},
    flaps_config=FlapOptions(stable=60, critical=300)
)

exports = [
    memento_get_all_objects_errors_alert,
    memento_get_all_objects_errors_check
]
