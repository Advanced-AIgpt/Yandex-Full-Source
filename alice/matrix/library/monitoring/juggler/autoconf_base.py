import copy
import hashlib
import os

from juggler_sdk import (
    Check,
    Child,
    FlapOptions,
    NotificationOptions,
)

import requests

import library.python.init_log
import logging


logger = logging.getLogger(__name__)
library.python.init_log.init_log(level='INFO')


class JugglerAutoConfBase:
    NAMESPACE = "matrix"

    STATUS_OK_TO_CRIT = {"from": "OK", "to": "CRIT"}
    STATUS_WARN_TO_CRIT = {"from": "WARN", "to": "CRIT"}
    STATUS_CRIT_TO_OK = {"from": "CRIT", "to": "OK"}

    SOLOMON_ALERTS_API = "https://solomon.yandex.net/api/v2/projects/matrix/alerts"
    YASM_ALERTS_API = "https://yasm.yandex-team.ru/srvambry/alerts"

    LOGINS = [
        "@svc_speechkit_ops_alice_notificator:duty",
        "chegoryu",
        "elshiko",
    ]

    CHATS = [
        "personal_cards",
    ]

    def __init__(
        self,
        service_name,
        docs_url,
    ):
        self.service_name = service_name
        self.docs_url = docs_url

    # Common
    def auto_managed_description(self):
        return f"Automatically managed by {self.__class__.__module__}.{self.__class__.__name__}"

    def format_cluster(self, cluster):
        if "_" in cluster:
            return cluster.split("_")[-1]

        return cluster

    def double_to_str(self, value):
        return f"to_fixed({value}, 2)"

    def get_percent(self, numenator, denominator):
        return f"100 * min([1.0, {numenator} / ({denominator} < 0.000001 ? 1.0 : {denominator})])"

    def get_percent_str(self, numenator, denominator):
        return self.double_to_str(self.get_percent(numenator, denominator))

    def get_real_alert_id(self, alert_id):
        return f"{self.service_name}.{alert_id}"

    def get_alert_id_hash(self, alert_id):
        return hashlib.md5(alert_id.encode("utf-8")).hexdigest()

    # Juggler
    def get_business_time_bounds(self):
        return {
            # NB: ignore_weekends is True by default
            "ignore_weekends": False,

            "time_start": "11:00",
            "time_end": "20:00",
        }

    def build_standart_notification(self, status, methods, business_time_only):
        template_kwargs = {
            "status": status,
            "login": self.LOGINS + self.CHATS,
            "method": methods,
        }

        if business_time_only:
            template_kwargs.update(self.get_business_time_bounds())

        if status in [self.STATUS_OK_TO_CRIT, self.STATUS_WARN_TO_CRIT]:
            template_kwargs["repeat"] = 172800

        return NotificationOptions(
            template_name="on_status_change",
            template_kwargs=template_kwargs,
            description=self.auto_managed_description(),
        )

    def build_escalation_notification(self, business_time_only):
        template_kwargs = {
            "logins": self.LOGINS,
            "delay": 30,
            "repeat": 10,
            "on_success_next_call_delay": 60,
        }

        if business_time_only:
            business_time_bounds = self.get_business_time_bounds()
            template_kwargs["time_start"] = business_time_bounds["time_start"]
            template_kwargs["time_end"] = business_time_bounds["time_end"]

        return NotificationOptions(
            template_name="phone_escalation",
            template_kwargs=template_kwargs,
            description=self.auto_managed_description(),
        )

    def build_notifications(self, is_prod):
        business_time_only = not is_prod
        methods = ["telegram", "email"]
        if is_prod:
            methods.append("sms")

        notifications = [
            self.build_standart_notification(
                status,
                methods,
                business_time_only
            ) for status in [self.STATUS_OK_TO_CRIT, self.STATUS_WARN_TO_CRIT, self.STATUS_CRIT_TO_OK]
        ]
        if is_prod:
            notifications.append(self.build_escalation_notification(business_time_only))

        return notifications

    def build_juggler_check(
        self,
        cluster,
        alert_id,
        name,
        is_prod,
        children,
        weak=False
    ):
        host = f"{self.service_name}-{self.format_cluster(cluster)}.yandex.net".replace("_", "-")
        service = f"{self.service_name}.{name}"

        check = Check(
            host=host,
            service=service,
            ttl=300,
            refresh_time=90,
            aggregator="logic_or",
            namespace=self.NAMESPACE,
            notifications=self.build_notifications(is_prod),
            meta={
                "urls": [
                    {
                        # Using wiki type for docs is not a mistake
                        # https://docs.yandex-team.ru/juggler/aggregates/basics#urls
                        "type": "wiki",
                        "url": self.docs_url,
                        "title": "Docs",
                    },
                    {
                        # Magic for screenshots: https://st.yandex-team.ru/JUGGLER-4359#61af0db80a52ef64539a1dd6
                        "type": "screenshot_url",
                        "url": "https://charts.yandex-team.ru/api/scr/v1/screenshots/preview/qv8q1s86j6r4o"
                               "?__json_error=1&__scr_height=720&__scr_width=1280&_embedded=1&_no_controls=1"
                               f"&_no_header=1&alertId={self.get_alert_id_hash(self.get_real_alert_id(alert_id))}"
                               f"&env=prod&projectId={self.NAMESPACE}",
                        "title": "Alert graph snapshot",
                    },
                ],
            },
            children=children,
            description=self.auto_managed_description(),
        )

        if weak:
            check.flaps_config = FlapOptions(stable=630, critical=1260, boost=0)

        logger.info("%s %s is_prod=%s weak=%s juggler check is built", host, service, is_prod, weak)

        return check

    # Golovan
    def create_or_update_golovan_check(self, check_name, check):
        logger.info("Try to update %s golovan check", check_name)
        result = requests.post(f"{self.YASM_ALERTS_API}/update?name={check_name}", json=check)
        if result.status_code == 404:
            logger.info("Check not found, try to create %s golovan check", check_name)
            result = requests.post(f"{self.YASM_ALERTS_API}/create?name={check_name}", json=check)

        try:
            result.raise_for_status()
            logger.info("%s golovan check created or updated", check_name)
        except:
            logger.exception("Failed to create or update %s golovan check", check_name)
            raise

    def build_balancer_total_fails_check(self, balancer_host, check_name, is_prod, crit_from=5):
        logger.info("%s %s is_prod=%s balancer total fails check is built", balancer_host, check_name, is_prod)

        return {
            "name": check_name,
            "signal": "div(balancer_report-report-service_total-fail_summ, normal())",
            "mgroups": [
                "ASEARCH",
            ],
            "tags": {
                "itype": [
                    "balancer",
                ],
                "prj": [
                    balancer_host,
                ]
            },
            "juggler_check": Check(
                refresh_time=5,
                tags=[
                    f"a_mark_yasm_{check_name}",
                    "a_itype_balancer",
                    f"a_prj_{balancer_host}",
                ],
                host=f"yasm_{check_name}",
                ttl=900,
                children=[
                    Child(
                        instance="",
                        host="yasm_alert",
                        service=check_name,
                        group_type="HOST",
                    ),
                ],
                service=check_name,
                aggregator="logic_or",
                namespace=self.NAMESPACE,
                notifications=self.build_notifications(is_prod=is_prod),
                aggregator_kwargs={
                    "unreach_service": [
                        {
                            "check": "yasm_alert:virtual-meta",
                        },
                    ],
                    "nodata_mode": "force_ok",
                    "unreach_mode": "force_ok",
                },
                flaps_config=FlapOptions(critical=200, boost=100, stable=100),
            ).to_dict(),
            "warn": [
                None,
                None,
            ],
            "crit": [
                crit_from,
                None,
            ]
        }

    # Solomon
    def create_or_update_solomon_alert(self, alert):
        alert = copy.deepcopy(alert)
        original_alert_id = alert["id"]
        alert_id = self.get_alert_id_hash(original_alert_id)
        alert["id"] = alert_id

        headers = {
            "Content-Type": "application/json",
            "Accept": "application/json",
            "Authorization": f"OAuth {os.getenv('SOLOMON_TOKEN')}",
        }

        logger.info("Try to get %s solomon alert", original_alert_id)

        get_result = requests.get(
            f"{self.SOLOMON_ALERTS_API}/{alert_id}",
            headers=headers,
        )

        if get_result.status_code == 404:
            logger.info("Alert not found, try to create %s solomon alert", original_alert_id)
            result = requests.post(
                f"{self.SOLOMON_ALERTS_API}",
                json=alert,
                headers=headers,
            )
        else:
            try:
                get_result.raise_for_status()
            except:
                logger.exception("Failed to get %s solomon alert", original_alert_id)
                raise

            alert["version"] = get_result.json()["version"]
            logger.info("Got %s solomon alert info, version id is %s, try to update", original_alert_id, alert["version"])

            result = requests.put(
                f"{self.SOLOMON_ALERTS_API}/{alert_id}",
                json=alert,
                headers=headers,
            )

        try:
            result.raise_for_status()
            logger.info("%s solomon alert created or updated", original_alert_id)
        except:
            logger.exception("Failed to create or update %s solomon alert, error: %s", original_alert_id, result.content)
            raise

    def build_default_solomon_sensor_selector(self, service, code, sensor, cluster):
        return f'{{service="{service}", code="{code}", sensor="{sensor}", cluster="{cluster}", host="cluster"}}'

    def build_solomon_alert_description_annotation(self, description_parts):
        return "\n    ".join(
            ["", "Period: [{{fromTime}}; {{toTime}}]"] + description_parts,
        )

    def build_solomon_check_percent_check_expression(self, percent, denominator=101):
        return f"errors / max([all, {denominator}]) > {percent}"

    def build_solomon_program_alert(
        self,
        alert_id,
        juggler_service_id,
        juggler_host,
        human_readable_name,
        program,
        check_expression,
        extra_annotations=None,
        resolved_empty_policy="RESOLVED_EMPTY_DEFAULT",
        group_by_labels=[],
    ):
        annotations = copy.deepcopy(extra_annotations or dict())
        annotations.update({
            "service": f"{self.service_name}.{juggler_service_id}",
            "host": juggler_host,
        })

        logger.info("%s %s %s '%s' solomon alert is built", alert_id, juggler_service_id, juggler_host, human_readable_name)

        return {
            "id": self.get_real_alert_id(alert_id),
            "projectId": self.NAMESPACE,
            "name": f"[{self.service_name}, autogenerated] {human_readable_name}",
            "version": 0,
            "groupByLabels": group_by_labels,
            "channels": [
                {
                    "id": "juggler",
                    "config": {
                        "notifyAboutStatuses": [
                            "ALARM",
                            "ERROR",
                            "OK",
                        ],
                        "repeatDelaySecs": 0
                    }
                }
            ],
            "annotations": annotations,
            "type": {
                "expression": {
                    "program": program,
                    "checkExpression": check_expression,
                }
            },
            "windowSecs": 300,
            "delaySecs": 0,
            "description": self.auto_managed_description(),
            "resolvedEmptyPolicy": resolved_empty_policy,
            "noPointsPolicy": "NO_POINTS_DEFAULT",
            "labels": {},
            "serviceProviderAnnotations": {},
        }

    def build_solomon_ydb_userpool_cpu_utilization_alert(
        self,
        alert_id,
        juggler_service_id,
        database,
    ):
        def get_database_userpool_cpu_lines(database, type_p):
            return f"""
                {{
                    project="kikimr",
                    cluster="ydb_ru_prestable|ydb_ru|ydb_eu",
                    host="cluster",
                    database="{database}",
                    service="ydb",
                    name="resources.cpu.{type_p}_core_percents",
                    pool="user"
                }}
            """

        return self.build_solomon_program_alert(
            alert_id,
            juggler_service_id,
            "ydb",
            f"YDB userpool CPU utilization {database}",
            f"""
                let usage_lines_sm = group_lines("sum", {get_database_userpool_cpu_lines(database, "used")});
                let limit_lines_sm = group_lines("sum", {get_database_userpool_cpu_lines(database, "limit")});

                let usage = avg(usage_lines_sm);
                let limit = avg(limit_lines_sm);

                let usage_percent = {self.get_percent("usage", "limit")};

                let usage_str = {self.double_to_str("usage")};
                let limit_str = {self.double_to_str("limit")};
                let usage_percent_str = {self.double_to_str("usage_percent")};
            """.replace("  ", ""),
            "usage_percent > 80.0",
            extra_annotations={
                "description": self.build_solomon_alert_description_annotation([
                    "Usage: {{expression.usage_str}} ({{expression.usage_percent_str}}%)",
                    "Limit: {{expression.limit_str}}",
                ]),
            },
        )

    def build_solomon_ydb_storage_utilization_alert(
        self,
        alert_id,
        juggler_service_id,
        database,
    ):
        def get_database_storage_lines(database, type_p):
            return f"""
                {{
                    project="kikimr",
                    cluster="ydb_ru_prestable|ydb_ru|ydb_eu",
                    host="cluster|-",
                    database="{database}",
                    service="ydb|ydb_serverless",
                    name="resources.storage.{type_p}_bytes"
                }}
            """

        return self.build_solomon_program_alert(
            alert_id,
            juggler_service_id,
            "ydb",
            f"YDB Storage utilization {database}",
            f"""
                let usage_lines_sm = group_lines("sum", {get_database_storage_lines(database, "used")});
                let limit_lines_sm = group_lines("sum", {get_database_storage_lines(database, "limit")});

                let usage = avg(usage_lines_sm);
                let limit = avg(limit_lines_sm);

                let usage_percent = {self.get_percent("usage", "limit")};

                let usage_gb_str = {self.double_to_str("usage / 1073741824")};
                let limit_gb_str = {self.double_to_str("limit / 1073741824")};
                let usage_percent_str = {self.double_to_str("usage_percent")};
            """.replace("  ", ""),
            "usage_percent > 70.0",
            extra_annotations={
                "description": self.build_solomon_alert_description_annotation([
                    "Usage (gb): {{expression.usage_gb_str}} ({{expression.usage_percent_str}}%)",
                    "Limit (gb): {{expression.limit_gb_str}}",
                ]),
            },
        )
