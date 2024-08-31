from dataclasses import dataclass
from typing import List, Dict

import six
from library.python.monitoring.solo.objects.solomon.v2 import Shard
from library.python.monitoring.solo.objects.solomon.sensor import Sensor

from alice.paskills.alerts.solomon_alerts.registry.shard import exports as shard_exports
from alice.paskills.alerts.solomon_alerts.registry.cluster.clusters import cluster_by_id
from alice.paskills.alerts.solomon_alerts.registry.service.services import service_by_id
from alice.paskills.alerts.solomon_alerts.registry.project.projects import project_by_id


class PerHostSensor(Sensor):

    def __str__(self):
        sequence = [self.format_label(k, v) for k, v in sorted(six.iteritems(self.labels))]
        sequence = ", ".join(sequence)
        return "".join(["{", sequence, "}"])

    def format_label(self, key: str, value: str) -> str:
        if key == 'host':
            return 'host!~"cluster|Man|Sas|Vla|dc-unknown"'
        else:
            return "\"{0}\"=\"{1}\"".format(key, value)


@dataclass
class JvmSensors:
    heap_used: Sensor
    heap_max: Sensor
    gc_time_ms: Sensor
    jvm_threads_deadlocked: Sensor

    def to_list(self) -> List[Sensor]:
        return [s for s in self.__dict__.values() if isinstance(s, Sensor)]


def make_jvm_sensors(shard: Shard) -> JvmSensors:
    print(f'processing shard {shard}')
    cluster = cluster_by_id[shard.cluster_id]
    service = service_by_id[shard.service_id]
    project = project_by_id[shard.project_id]
    shard_kwargs = {
        'project': project.name,
        'cluster': cluster.name,
        'service': service.name,
        'host': '*',
    }
    return JvmSensors(
        PerHostSensor(
            sensor='jvm.memory.used',
            memory='heap',
            **shard_kwargs,
        ),
        PerHostSensor(
            sensor='jvm.memory.max',
            memory='heap',
            **shard_kwargs,
        ),
        PerHostSensor(
            sensor='jvm.gc.timeMs',
            gc='Shenandoah Pauses',
            **shard_kwargs,
        ),
        PerHostSensor(
            sensor='jvm.threads.deadlocked',
            **shard_kwargs,
        )
    )


jvm_sensors_by_shard: Dict[str, JvmSensors] = {shard.id: make_jvm_sensors(shard) for shard in shard_exports}
