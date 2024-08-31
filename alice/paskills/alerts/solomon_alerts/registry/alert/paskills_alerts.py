import datetime
from typing import List

from juggler_sdk import Check
from library.python.monitoring.solo.objects.solomon.v2 import Shard
from library.python.monitoring.solo.objects.solomon.v2 import Alert, Type, Expression, MultiAlert

from alice.paskills.alerts.solomon_alerts.registry.channel.channels import paskills_juggler
from alice.paskills.alerts.solomon_alerts.registry.project.projects import paskills_alerts
from alice.paskills.alerts.solomon_alerts.registry.sensor.paskills_sensors import JvmSensors, make_jvm_sensors
from alice.paskills.alerts.solomon_alerts.registry.shard.shards import jvm_metric_shards


def make_alert_check(alert):
    return Check(
        host=alert.annotations["host"],
        service=alert.annotations["service"],
        namespace="paskills",
        refresh_time=60, ttl=600,
        aggregator="logic_or",
        tags={'telegram'}
    )


def make_jvm_alerts(shard: Shard, jvm_sensors: JvmSensors) -> List[Alert]:
    default_kwargs = dict(
        group_by_labels={'host'},
        window_secs=int(datetime.timedelta(minutes=5).total_seconds()),
        notification_channels={paskills_juggler.id},
    )
    return [
        MultiAlert(
            id=f'{shard.id}_heap_usage',
            name=f'JVM heap usage ({shard.id})',
            project_id=paskills_alerts.id,
            type=Type(
                expression=Expression(program=f"""
                    let used = {jvm_sensors.heap_used};
                    let max = {jvm_sensors.heap_max};
                    let last_used_perc = last(used / max);
                    alarm_if(last_used_perc > 0.90);
                """)
            ),
            annotations={
                'service': 'jvm_heap_usage',
                'host': f'{shard.id}',
            },
            **default_kwargs,
        ),
        MultiAlert(
            id=f'{shard.id}_gc_pause_ms',
            name=f'GC Pause (ms) ({shard.id})',
            project_id=paskills_alerts.id,
            type=Type(
                expression=Expression(
                    program=f"""
                        let pause = last({jvm_sensors.gc_time_ms});
                        alarm_if(pause > 25);
                    """
                )
            ),
            annotations={
                'service': 'jvm_gc_pause_ms',
                'host': f'{shard.id}',
            },
            **default_kwargs,
        ),
        MultiAlert(
            id=f'{shard.id}_threads_deadlocked',
            name=f'Deadlocked thread count ({shard.id})',
            project_id=paskills_alerts.id,
            type=Type(
                expression=Expression(
                    program=f"""
                        let deadlocked_thread_count = last({jvm_sensors.jvm_threads_deadlocked});
                        alarm_if(deadlocked_thread_count > 0);
                    """
                )
            ),
            annotations={
                'service': 'jvm_threads_deadlocked',
                'host': f'{shard.id}',
            },
            **default_kwargs,
        ),
    ]


exports = [
]


for shard in jvm_metric_shards:
    sensors = make_jvm_sensors(shard)
    for alert in make_jvm_alerts(shard, sensors):
        check = make_alert_check(alert)
        exports.append(alert)
        exports.append(check)
