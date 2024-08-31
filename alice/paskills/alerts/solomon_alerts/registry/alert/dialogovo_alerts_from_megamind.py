import datetime

from juggler_sdk import Check, FlapOptions
from library.python.monitoring.solo.objects.solomon.v2 import MultiAlert, Type, Expression

from alice.paskills.alerts.solomon_alerts.registry.channel.channels import dialogovo_juggler
from alice.paskills.alerts.solomon_alerts.registry.project.projects import dialogovo
from alice.paskills.alerts.solomon_alerts.registry.sensor.megamind_sensors import dialogovo_scenario_response_type_is_error, \
    dialogovo_scenario_response_total
from alice.paskills.alerts.solomon_alerts.registry.shard.shards import dialogovo_prod_main_pulling

skill_player_errors_alert = MultiAlert(
    id='dialogovo_scenario_response_type_is_error',
    name='Dialogovo scenario response errors rate',
    description='Тип ответа "ошибка" для сценария Dialogovo со стороны megamind',
    project_id=dialogovo.id,
    type=Type(
        expression=Expression(program=f"""
                    let errors = {dialogovo_scenario_response_type_is_error};
                    let total = {dialogovo_scenario_response_total};

                    let percent = sum(group_lines("sum", errors)) / sum(group_lines("sum", total));
                    let maxval = max(group_lines("sum", total));

                    let threshold = maxval > 2 ? 0.07 : (maxval > 1 ? 0.3 : 0.5);
                    alarm_if(percent >= threshold);
                    let threshold = maxval > 2 ? 0.05 : (maxval > 1 ? 0.2 : 0.3);
                    warn_if(percent >= threshold);
                """)
    ),
    delay_seconds=0,
    annotations={
        "graphLink": "https://solomon.yandex-team.ru/?project=megamind&cluster=prod&service=server&l.\
        scenario_name=DIALOGOVO&l.name=scenario.protocol.responses_per_second&l.host=cluster&graph=auto&l.\
        request_type=%7B%7Blabels.request_type%7D%7D&b=1d&l.response_type=error*",
        'service': 'dialogovo_scenario_response_type_is_error',
        'host': dialogovo_prod_main_pulling.id
    },
    window_secs=int(datetime.timedelta(seconds=30).total_seconds()),
    group_by_labels={'request_type'},
    notification_channels={dialogovo_juggler.id}
)

skill_player_errors_alert_check = Check(
    host=skill_player_errors_alert.annotations["host"],
    service=skill_player_errors_alert.annotations["service"],
    namespace="paskills",
    refresh_time=30, ttl=600,
    aggregator="logic_or",
    tags={'telegram'},
    flaps_config=FlapOptions(stable=120, critical=300)
)

exports = [
    skill_player_errors_alert,
    skill_player_errors_alert_check
]
