import datetime

from juggler_sdk import Check, FlapOptions
from library.python.monitoring.solo.objects.solomon.v2 import MultiAlert, Type, Threshold, PredicateRule, Expression,\
    Alert

from alice.paskills.alerts.solomon_alerts.registry.channel.channels import dialogovo_juggler
from alice.paskills.alerts.solomon_alerts.registry.project.projects import dialogovo
from alice.paskills.alerts.solomon_alerts.registry.sensor.dialogovo_sensors import ydb_errors, skill_player_errors,\
    skill_player_requests, radionews_wt_disney_content_delay, radionews_disney_content_delay, \
    megamind_apply_http_errors, megamind_apply_http_requests, megamind_run_http_errors, megamind_run_http_requests, \
    zora_errors, zora_requests, zora_timeouts, thread_pool_rejects, thread_pool_queue_remaining_capacity, \
    dialogovo_sensor_limit, dialogovo_sensor_usage, penguinary_sensor_limit, penguinary_sensor_usage
from alice.paskills.alerts.solomon_alerts.registry.shard.shards import dialogovo_prod_main_pulling

ydb_alert = MultiAlert(
    id='ydb_queries_errors',
    name='YDB queries errors rate is too high',
    project_id=dialogovo.id,
    type=Type(
        threshold=Threshold(
            selectors=ydb_errors.selectors,
            time_aggregation="AT_LEAST_ONE",
            predicate="GT",
            threshold=1,
            predicate_rules=[
                PredicateRule(
                    threshold_type="AT_LEAST_ONE",
                    comparison="GT",
                    threshold=1,
                    target_status="ALARM"
                ),
                PredicateRule(
                    threshold_type="AT_LEAST_ONE",
                    comparison="GT",
                    threshold=0.5,
                    target_status="WARN"
                ),
            ]
        )
    ),
    delay_seconds=0,
    annotations={
        "graphLink": "https://solomon.yandex-team.ru/?project=dialogovo&cluster=prod&service=main_pulling&l.sensor=ydb.*errors&l.host=cluster&graph=auto",
        'service': 'dialogovo_ydb_errors',
        'host': dialogovo_prod_main_pulling.id
    },
    window_secs=int(datetime.timedelta(minutes=5).total_seconds()),
    group_by_labels={'query'},
    notification_channels={dialogovo_juggler.id}
)

ydb_alert_check = Check(
    host=ydb_alert.annotations["host"],
    service=ydb_alert.annotations["service"],
    namespace="paskills",
    refresh_time=60, ttl=600,
    aggregator="logic_or",
    flaps_config=FlapOptions(stable=60, critical=300),
    tags={'telegram'}
)


skill_player_errors_alert = MultiAlert(
    id='skill_player_errors',
    name='Skill player errors rate',
    project_id=dialogovo.id,
    type=Type(
        expression=Expression(program=f"""
                    let player_errors = sum(group_lines("sum", {skill_player_errors}));
                    let player_requests = sum(group_lines("sum", {skill_player_requests}));
                    let player_errors_perc = player_errors / player_requests * 100;

                    alarm_if(player_errors_perc > 5 && player_requests >= 3);
                    warn_if(player_errors_perc > 2 && player_requests >= 3);
                """)
    ),
    delay_seconds=0,
    annotations={
        "graphLink": "https://solomon.yandex-team.ru/?project=quasar&cluster=all_devices%7C*&service=aggregated_metrics\
                     &MetricName=audioClientError*.count&player_name=external_skill__audio_play__*&graph=auto",
        'service': 'dialogovo_skill_player_errors',
        'host': dialogovo_prod_main_pulling.id
    },
    window_secs=int(datetime.timedelta(minutes=5).total_seconds()),
    group_by_labels={'player_name'},
    notification_channels={dialogovo_juggler.id}
)

skill_player_errors_check = Check(
    host=skill_player_errors_alert.annotations["host"],
    service=skill_player_errors_alert.annotations["service"],
    namespace="paskills",
    refresh_time=60, ttl=600,
    aggregator="logic_or",
    tags={'telegram'}
)

radionews_wt_disney_staleness_alert = MultiAlert(
    id='radionews_most_fresh_content_delay_sec',
    name='Отставание самого свежего контента радионовостей',
    project_id=dialogovo.id,
    type=Type(
        threshold=Threshold(
            selectors=radionews_wt_disney_content_delay.selectors,
            time_aggregation="AT_LEAST_ONE",
            predicate="GT",
            threshold=172800,
            predicate_rules=[
                PredicateRule(
                    threshold_type="AT_LEAST_ONE",
                    comparison="GT",
                    threshold=172800,
                    target_status="ALARM"
                )
            ]
        )
    ),
    delay_seconds=0,
    annotations={
        "graphLink": "https://solomon.yandex-team.ru/?project=dialogovo&cluster=prod&service=main_pulling&l.sensor=\
        news.most.fresh.content.delay.sec&graph=auto&l.host=dialogovo-man-1",
        'service': 'radionews_wt_disney_staleness_alert',
        'host': dialogovo_prod_main_pulling.id
    },
    window_secs=int(datetime.timedelta(minutes=5).total_seconds()),
    group_by_labels={'feed'},
    notification_channels={"voicetech_dialog_monitoring_telegram", "radionew_with_producst_channel"}
)

radionews_disney_staleness_alert = MultiAlert(
    id='disney_radionews_most_fresh_content_delay_sec',
    name='Отставание самого свежего контента радионовостей от Дисней',
    project_id=dialogovo.id,
    type=Type(
        threshold=Threshold(
            selectors=radionews_disney_content_delay.selectors,
            time_aggregation="AT_LEAST_ONE",
            predicate="GT",
            threshold=432000,
            predicate_rules=[
                PredicateRule(
                    threshold_type="AT_LEAST_ONE",
                    comparison="GT",
                    threshold=432000,
                    target_status="ALARM"
                )
            ]
        )
    ),
    delay_seconds=0,
    annotations={
        "graphLink": "https://solomon.yandex-team.ru/?project=dialogovo&cluster=prod&service=main_pulling&l.sensor=\
        news.most.fresh.content.delay.sec&graph=auto&l.host=dialogovo-man-1",
        'service': 'radionews_disney_staleness_alert',
        'host': dialogovo_prod_main_pulling.id
    },
    window_secs=int(datetime.timedelta(minutes=5).total_seconds()),
    group_by_labels={'feed'},
    notification_channels={"voicetech_dialog_monitoring_telegram", "radionew_with_producst_channel"}
)

http_errors_rate_megamind_run_alert = Alert(
    id='http_in_requests_failure_rate_megamind_run',
    name='Megamind controller RUN http errors percent is too high',
    description='Megamind Controller RUN cluster wide non-ok http codes rate threshold excess',
    project_id=dialogovo.id,
    type=Type(
        expression=Expression(program=f"""
                    let errors_perc = {megamind_run_http_errors} / {megamind_run_http_requests} * 100;
                    let errors_perc_avg = avg(errors_perc);

                    alarm_if(errors_perc_avg >= 5);
                    warn_if(errors_perc_avg >= 2);
                """)
    ),
    delay_seconds=0,
    annotations={
        "graphLink": 'https://solomon.yandex-team.ru/?project=dialogovo&cluster=prod&service=main_pulling&graph=http_in_requests_failure_megamind_run_percent',
        'currentValue': '{{expression.errors_perc_avg}}',
        'service': 'http_in_requests_failure_rate_megamind_run',
        'host': dialogovo_prod_main_pulling.id
    },
    window_secs=int(datetime.timedelta(minutes=1).total_seconds()),
    notification_channels={dialogovo_juggler.id}
)

http_errors_rate_megamind_run_check = Check(
    host=http_errors_rate_megamind_run_alert.annotations["host"],
    service=http_errors_rate_megamind_run_alert.annotations["service"],
    namespace="paskills",
    refresh_time=60, ttl=600,
    aggregator="logic_or",
    tags={'telegram'}
)

http_errors_rate_megamind_apply_alert = Alert(
    id='http_in_requests_failure_rate_megamind_apply',
    name='Megamind controller APPLY http errors percent is too high',
    description='Megamind Controller APPLY cluster wide non-ok http codes rate threshold excess',
    project_id=dialogovo.id,
    type=Type(
        expression=Expression(program=f"""
                    let errors_perc = {megamind_apply_http_errors} / {megamind_apply_http_requests} * 100;
                    let errors_perc_avg = avg(errors_perc);

                    alarm_if(errors_perc_avg >= 5);
                    warn_if(errors_perc_avg >= 2);
                """)
    ),
    delay_seconds=0,
    annotations={
        "graphLink": 'https://solomon.yandex-team.ru/?project=dialogovo&cluster=prod&service=main_pulling&graph=http_in_requests_failure_megamind_apply_percent',
        'currentValue': '{{expression.errors_perc_avg}}',
        'service': 'http_in_requests_failure_rate_megamind_apply',
        'host': dialogovo_prod_main_pulling.id
    },
    window_secs=int(datetime.timedelta(minutes=1).total_seconds()),
    notification_channels={dialogovo_juggler.id}
)

http_errors_rate_megamind_apply_check = Check(
    host=http_errors_rate_megamind_apply_alert.annotations["host"],
    service=http_errors_rate_megamind_apply_alert.annotations["service"],
    namespace="paskills",
    refresh_time=60, ttl=600,
    aggregator="logic_or",
    tags={'telegram'}
)

zora_online_errors_alert = Alert(
    id='zora_error',
    name='Количество ошибок Zora Online',
    project_id=dialogovo.id,
    type=Type(
        expression=Expression(
            program=f"""
            let zora_errors = {zora_errors};
            let total_requests = {zora_requests};

            let error_perc = zora_errors / total_requests * 100;
            """,
            check_expression="avg(error_perc) >= 1 && avg(zora_errors) >= 0.5")
    ),
    delay_seconds=180,
    annotations={
        "graphLink": 'https://solomon.yandex-team.ru/?project=dialogovo&cluster=prod&service=main_pulling&host=cluster&sensor=zora_error&skill_id=total&graph=auto',
        'currentValue': '{{avg(expression.error_perc)}}',
        'service': 'zora_error',
        'host': dialogovo_prod_main_pulling.id
    },
    window_secs=int(datetime.timedelta(minutes=5).total_seconds()),
    notification_channels={dialogovo_juggler.id}
)

zora_online_errors_check = Check(
    host=zora_online_errors_alert.annotations["host"],
    service=zora_online_errors_alert.annotations["service"],
    namespace="paskills",
    refresh_time=60, ttl=600,
    aggregator="logic_or",
    tags={'telegram'}
)

zora_online_timeouts_alert = Alert(
    id='zora_timeout',
    name='Количество таймаутов Zora Online',
    project_id=dialogovo.id,
    type=Type(
        expression=Expression(
            program=f"""
            let zora_timeouts = {zora_timeouts};
            let total_requests = {zora_requests};

            let timeout_perc = zora_timeouts / total_requests * 100;
            """,
            check_expression="avg(timeout_perc) >= 1 && avg(zora_timeouts) >= 15")
    ),
    delay_seconds=180,
    annotations={
        "graphLink": 'https://solomon.yandex-team.ru/?project=dialogovo&cluster=prod&service=main_pulling&host=cluster&sensor=zora_timeout&skill_id=total&graph=auto',
        'currentValue': '{{avg(expression.timeout_perc)}}',
        'service': 'zora_timeout',
        'host': dialogovo_prod_main_pulling.id
    },
    window_secs=int(datetime.timedelta(minutes=5).total_seconds()),
    notification_channels={dialogovo_juggler.id}
)

zora_online_timeouts_check = Check(
    host=zora_online_timeouts_alert.annotations["host"],
    service=zora_online_timeouts_alert.annotations["service"],
    namespace="paskills",
    refresh_time=60, ttl=600,
    aggregator="logic_or",
    tags={'telegram'}
)

thread_pool_reject_rate_alert = MultiAlert(
    id='thread_pool_reject_rate',
    name='Thread Pool tasks reject rate found',
    project_id=dialogovo.id,
    type=Type(
        threshold=Threshold(
            selectors=thread_pool_rejects.selectors,
            time_aggregation="AT_LEAST_ONE",
            predicate="GT",
            threshold=0,
            predicate_rules=[
                PredicateRule(
                    threshold_type="AT_LEAST_ONE",
                    comparison="GT",
                    threshold=0,
                    target_status="ALARM"
                ),
            ]
        )
    ),
    delay_seconds=0,
    annotations={
        "graphLink": "https://solomon.yandex-team.ru/?project=dialogovo&cluster=prod&service=main_pulling&sensor=thread.rejected&host=cluster&graph=auto",
        'service': 'thread_pool_reject',
        'host': dialogovo_prod_main_pulling.id
    },
    window_secs=int(datetime.timedelta(minutes=5).total_seconds()),
    group_by_labels={'pool'},
    notification_channels={dialogovo_juggler.id}
)

thread_pool_reject_rate_check = Check(
    host=thread_pool_reject_rate_alert.annotations["host"],
    service=thread_pool_reject_rate_alert.annotations["service"],
    namespace="paskills",
    refresh_time=60, ttl=600,
    aggregator="logic_or",
    tags={'telegram'}
)

thread_pool_queue_remaining_capacity_alert = MultiAlert(
    id='thread_pool_queue_remaining_capacity_low',
    name='Thread pool queue remaining capacity is too low',
    project_id=dialogovo.id,
    type=Type(
        threshold=Threshold(
            selectors=thread_pool_queue_remaining_capacity.selectors,
            time_aggregation="AT_LEAST_ONE",
            predicate="LT",
            threshold=50,
            predicate_rules=[
                PredicateRule(
                    threshold_type="AT_LEAST_ONE",
                    comparison="LT",
                    threshold=50,
                    target_status="WARN"
                ),
                PredicateRule(
                    threshold_type="AT_LEAST_ONE",
                    comparison="LT",
                    threshold=10,
                    target_status="ALARM"
                ),
            ]
        )
    ),
    delay_seconds=0,
    annotations={
        "graphLink": "https://solomon.yandex-team.ru/?project=dialogovo&cluster=prod&service=main_pulling&sensor=thread.queueRemainingCapacity&host=cluster&graph=auto",
        'service': 'thread_pool_queue_remaining_capacity',
        'host': dialogovo_prod_main_pulling.id
    },
    window_secs=int(datetime.timedelta(minutes=5).total_seconds()),
    group_by_labels={'pool'},
    notification_channels={dialogovo_juggler.id}
)

thread_pool_queue_remaining_capacity_check = Check(
    host=thread_pool_queue_remaining_capacity_alert.annotations["host"],
    service=thread_pool_queue_remaining_capacity_alert.annotations["service"],
    namespace="paskills",
    refresh_time=60, ttl=600,
    aggregator="logic_or",
    tags={'telegram'}
)

dialogovo_sensor_count_alert = Alert(
    id='sensor-count-alert',
    name='Solomon Dialogovo sensor count limit alert',
    description='Проверка количества сенсоров',
    project_id=dialogovo.id,
    type=Type(
        expression=Expression(program=f"""
                    let limit = {dialogovo_sensor_limit};
                    let usage = {dialogovo_sensor_usage};

                    let avg_usage = avg(usage);
                    let avg_limit = avg(limit);

                    alarm_if(avg_usage/avg_limit > 0.9);
                    warn_if(avg_usage/avg_limit > 0.8);
                """)
    ),
    delay_seconds=0,
    annotations={
        'service': 'dialogovo_sensor_count_alert',
        'host': dialogovo_prod_main_pulling.id
    },
    window_secs=int(datetime.timedelta(days=1).total_seconds()),
    notification_channels={dialogovo_juggler.id}
)

dialogovo_sensor_count_check = Check(
    host=dialogovo_sensor_count_alert.annotations["host"],
    service=dialogovo_sensor_count_alert.annotations["service"],
    namespace="paskills",
    refresh_time=3600, ttl=10800,
    aggregator="logic_or",
    tags={'telegram'}
)

penguinary_sensor_count_alert = Alert(
    id='penguinary-sensor-count-alert',
    name='Solomon Penguinary sensor count limit alert',
    description='Проверка количества сенсоров от пингвинария',
    project_id=dialogovo.id,
    type=Type(
        expression=Expression(program=f"""
                    let limit = {penguinary_sensor_limit};
                    let usage = {penguinary_sensor_usage};

                    let avg_usage = avg(usage);
                    let avg_limit = avg(limit);

                    alarm_if(avg_usage/avg_limit > 0.9);
                    warn_if(avg_usage/avg_limit > 0.8);
                """)
    ),
    delay_seconds=0,
    annotations={
        'service': 'penguinary_sensor_count_alert',
        'host': dialogovo_prod_main_pulling.id
    },
    window_secs=int(datetime.timedelta(days=1).total_seconds()),
    notification_channels={dialogovo_juggler.id}
)

penguinary_sensor_count_check = Check(
    host=penguinary_sensor_count_alert.annotations["host"],
    service=penguinary_sensor_count_alert.annotations["service"],
    namespace="paskills",
    refresh_time=3600, ttl=10800,
    aggregator="logic_or",
    tags={'telegram'}
)

exports = [
    ydb_alert,
    ydb_alert_check,
    skill_player_errors_alert,
    skill_player_errors_check,
    radionews_wt_disney_staleness_alert,
    radionews_disney_staleness_alert,
    http_errors_rate_megamind_run_alert,
    http_errors_rate_megamind_run_check,
    http_errors_rate_megamind_apply_alert,
    http_errors_rate_megamind_apply_check,
    zora_online_errors_alert,
    zora_online_errors_check,
    zora_online_timeouts_alert,
    zora_online_timeouts_check,
    thread_pool_reject_rate_alert,
    thread_pool_reject_rate_check,
    thread_pool_queue_remaining_capacity_alert,
    thread_pool_queue_remaining_capacity_check,
    dialogovo_sensor_count_alert,
    penguinary_sensor_count_alert,
    penguinary_sensor_count_check
]
