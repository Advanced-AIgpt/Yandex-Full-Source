from library.python.monitoring.solo.objects.solomon.sensor import Sensor

from alice.paskills.alerts.solomon_alerts.registry.project.projects import dialogovo
from alice.paskills.alerts.solomon_alerts.registry.cluster.clusters import dialogovo_prod
from alice.paskills.alerts.solomon_alerts.registry.service.services import dialogovo_main_pulling


ydb_errors = Sensor(
    project=dialogovo.name,
    cluster=dialogovo_prod.name,
    service=dialogovo_main_pulling.name,
    sensor='ydb.*errors',
    kind="DGAUGE",
    host='cluster'
)

skill_player_errors = Sensor(
    project="quasar",
    cluster='*',
    service='aggregated_metrics',
    MetricName='audioClientError.count',
    player_name='external_skill__audio_play__*'
)

skill_player_requests = Sensor(
    project="quasar",
    cluster='*',
    service='aggregated_metrics',
    MetricName='audioClientPlayRequest.count',
    player_name='external_skill__audio_play__*'
)

radionews_wt_disney_content_delay = Sensor(
    project=dialogovo.name,
    cluster=dialogovo_prod.name,
    service=dialogovo_main_pulling.name,
    sensor='news.most.fresh.content.delay.sec',
    host='dialogovo-man-1',
    **{"feed!": "74ee5150-disnej.main"}
)

radionews_disney_content_delay = Sensor(
    project=dialogovo.name,
    cluster=dialogovo_prod.name,
    service=dialogovo_main_pulling.name,
    sensor='news.most.fresh.content.delay.sec',
    host='dialogovo-man-1',
    **{"feed": "74ee5150-disnej.main"}
)

megamind_run_http_errors = Sensor(
    project=dialogovo.name,
    cluster=dialogovo_prod.name,
    service=dialogovo_main_pulling.name,
    sensor='http.in.requests_failure_rate',
    path='megamind_run',
    host='cluster'
)

megamind_run_http_requests = Sensor(
    project=dialogovo.name,
    cluster=dialogovo_prod.name,
    service=dialogovo_main_pulling.name,
    sensor='http.in.requests_rate',
    path='megamind_run',
    host='cluster'
)

megamind_apply_http_errors = Sensor(
    project=dialogovo.name,
    cluster=dialogovo_prod.name,
    service=dialogovo_main_pulling.name,
    sensor='http.in.requests_failure_rate',
    path='megamind_apply',
    host='cluster'
)

megamind_apply_http_requests = Sensor(
    project=dialogovo.name,
    cluster=dialogovo_prod.name,
    service=dialogovo_main_pulling.name,
    sensor='http.in.requests_rate',
    path='megamind_apply',
    host='cluster'
)

zora_errors = Sensor(
    project=dialogovo.name,
    cluster=dialogovo_prod.name,
    service=dialogovo_main_pulling.name,
    sensor='zora_error',
    skill_id='total',
    host='cluster'
)

zora_timeouts = Sensor(
    project=dialogovo.name,
    cluster=dialogovo_prod.name,
    service=dialogovo_main_pulling.name,
    sensor='zora_timeout',
    skill_id='total',
    host='cluster'
)

zora_requests = Sensor(
    project=dialogovo.name,
    cluster=dialogovo_prod.name,
    service=dialogovo_main_pulling.name,
    target="webhook.zora",
    sensor='http.out.requests.invocations',
    host='cluster'
)

thread_pool_rejects = Sensor(
    project=dialogovo.name,
    cluster=dialogovo_prod.name,
    service=dialogovo_main_pulling.name,
    sensor='thread.rejected',
    host='cluster'
)

thread_pool_queue_remaining_capacity = Sensor(
    project=dialogovo.name,
    cluster=dialogovo_prod.name,
    service=dialogovo_main_pulling.name,
    sensor='thread.queueRemainingCapacity',
    host='cluster'
)

dialogovo_sensor_limit = Sensor(
    project='solomon',
    projectId=dialogovo.id,
    cluster='production',
    service='coremon',
    sensor='engine.fileSensorsLimit',
    host='cluster',
    shardId='dialogovo_prod_main_pulling',
)

dialogovo_sensor_usage = Sensor(
    project='solomon',
    projectId=dialogovo.id,
    cluster='production',
    service='coremon',
    sensor='engine.fileSensors',
    host='cluster',
    shardId='dialogovo_prod_main_pulling',
)

penguinary_sensor_limit = Sensor(
    project='solomon',
    projectId=dialogovo.id,
    cluster='production',
    service='coremon',
    sensor='engine.fileSensorsLimit',
    host='cluster',
    shardId='dialogovo_pengd_prod_penguinary',
)

penguinary_sensor_usage = Sensor(
    project='solomon',
    projectId=dialogovo.id,
    cluster='production',
    service='coremon',
    sensor='engine.fileSensors',
    host='cluster',
    shardId='dialogovo_pengd_prod_penguinary',
)
