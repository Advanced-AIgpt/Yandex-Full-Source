from juggler_sdk import (
    Check,
    Child,
    JugglerApi,
    NotificationOptions,
)

import os

TESTING_CLUSTERS = ["testing"]
PRESTABLE_CLUSTERS = ["prestable"]
PROD_CLUSTERS = ["sas", "man", "vla"]

status_ok_to_crit = {"from": "OK", "to": "CRIT"}
status_warn_to_crit = {"from": "WARN", "to": "CRIT"}
status_crit_to_ok = {"from": "CRIT", "to": "OK"}


def auto_managed_description():
    return "Automatically managed by alice/personal_cards/monitoring/juggler"


LOGINS = [
    "@svc_yandexassistantcards:duty",
    "chegoryu",
    "elshiko",
]


def get_business_time_bounds():
    return {
        "time_start": "11:00",
        "time_end": "20:00"
    }


def build_standart_notification(status, methods, business_time_only):
    template_kwargs = {
        "status": status,
        "login": LOGINS + ["personal_cards"],
        "method": methods
    }

    if business_time_only:
        business_time_bounds = get_business_time_bounds()
        template_kwargs["time_start"] = business_time_bounds["time_start"]
        template_kwargs["time_end"] = business_time_bounds["time_end"]

    if status in [status_ok_to_crit, status_warn_to_crit]:
        template_kwargs["repeat"] = 172800

    return NotificationOptions(
        template_name="on_status_change",
        template_kwargs=template_kwargs,
        description=auto_managed_description(),
    )


def build_escalation_notification(business_time_only):
    template_kwargs={
        "logins": LOGINS,
        "delay": 30,
        "repeat": 10,
        "on_success_next_call_delay": 60
    }

    if business_time_only:
        business_time_bounds = get_business_time_bounds()
        template_kwargs["time_start"] = business_time_bounds["time_start"]
        template_kwargs["time_end"] = business_time_bounds["time_end"]

    return NotificationOptions(
        template_name="phone_escalation",
        template_kwargs=template_kwargs,
        description=auto_managed_description(),
    )


def build_notifications(is_prod):
    business_time_only = not is_prod
    methods = ["telegram", "sms", "email"]

    notifications = [
        build_standart_notification(
            status,
            methods,
            business_time_only
        ) for status in [status_ok_to_crit, status_warn_to_crit, status_crit_to_ok]
    ]
    if is_prod:
        notifications.append(build_escalation_notification(business_time_only))

    return notifications


def create_check(cluster, is_prod, children):
    return Check(
        host="personal-cards-{}.yandex.net".format(cluster),
        service="personal-cards",
        ttl=300,
        refresh_time=90,
        aggregator="logic_or",
        namespace="personal-cards",
        notifications=build_notifications(is_prod),
        meta={
            "urls": [{
                "type": "wiki",
                "url": "https://wiki.yandex-team.ru/users/zhigan/personal-cards/",
                "title": "Personal cards",
            }]
        },
        children=children,
        description=auto_managed_description()
    )


def ydb():
    alerts = [
        "ydb_cpu_load",
        "ydb_disk_usage",
        "ydb_query_error_percentage",
    ]

    return create_check(
        cluster="ydb",
        is_prod=True,
        children=[
            Child(
                host="(host=ydb)&service=personal_cards.{}".format(alert),
                service="all",
                group_type="EVENTS",
                instance="all",
            ) for alert in alerts
        ]
    )


def personal_cards(cluster, is_prod):
    alerts = [
        "http_response_2XX",
        "http_request_exception",
        "get_request_time_limit_exceeded",
        "add_request_time_limit_exceeded",
        "remove_request_time_limit_exceeded",
        "deprecated_routes_usage",
        "get_stories_info_route_usage",
        "dismiss_request_proto_parse",
        "push_card_proto_parse",
        "request_has_exactly_one_user_id",
        "memory_lock_is_off",
    ]

    return create_check(
        cluster=cluster,
        is_prod=is_prod,
        children=[
            Child(
                host="(host={})&service=personal_cards.{}".format(cluster, alert),
                service="all",
                group_type="EVENTS",
                instance="all",
            ) for alert in alerts
        ],
    )


def main():
    with JugglerApi("http://juggler-api.search.yandex.net",
                    mark="personal_cards_checks",
                    oauth_token=os.environ["JUGGLER_TOKEN"]) as api:

        for cluster in TESTING_CLUSTERS:
            api.upsert_check(personal_cards(cluster, is_prod=False))

        for cluster in PRESTABLE_CLUSTERS:
            api.upsert_check(personal_cards(cluster, is_prod=True))

        for cluster in PROD_CLUSTERS:
            api.upsert_check(personal_cards(cluster, is_prod=True))

        # YDB
        api.upsert_check(ydb())


if __name__ == "__main__":
    main()
