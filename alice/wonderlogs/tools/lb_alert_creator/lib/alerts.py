import hashlib
import uuid
from typing import Optional

from alice.wonderlogs.tools.lb_alert_creator.lib.config import Config
from alice.wonderlogs.tools.lb_alert_creator.lib.entities import (
    PredicateRule, Selector, ThresholdType, Comparison, Status, Expression, Alert, Channel, Annotation, Threshold,
    Program, Topic)

DEFAULT_WINDOW_SECS = 300

COMMON_PREFIX = 'autogen'


def topic_path_to_name(name: str) -> str:
    return name.lstrip('/').lower().replace('/', '-')


def get_common_selectors() -> list[Selector]:
    return [Selector(label='cluster', value='lbk'),
            Selector(label='project', value='kikimr')]


def get_account_topic_path_selectors(account: str, topic_paths: list[str]) -> list[Selector]:
    return [
        Selector(label='Account', value=account),
        Selector(label='TopicPath', value='|'.join(topic_paths))
    ]


def get_read_common_selectors(account: str, topic: Topic) -> list[Selector]:
    selectors = get_common_selectors()
    selectors.extend(get_account_topic_path_selectors(account, [topic.topic_path]))
    selectors.append(Selector(label='ConsumerPath', value='|'.join(topic.consumer_paths)))
    return selectors


def get_write_common_threshold_selectors(account: str, topic_paths: list[str]) -> list[Selector]:
    selectors = get_common_selectors()
    selectors.extend(get_account_topic_path_selectors(account, topic_paths))
    return selectors


def generate_alert(name: str, project_id: str, channels: list[Channel], window_secs: int,
                   group_by_labels: Optional[list[str]], annotations: Optional[list[Annotation]]) -> Alert:
    id = str(uuid.UUID(hashlib.md5(name.encode('utf-8')).hexdigest()))
    alert = Alert(id, project_id, name, None, channels, None, window_secs, group_by_labels, 0, annotations)
    return alert


def generate_threshold_alert(name: str, project_id: str, channels: list[Channel], predicate_rules: list[PredicateRule],
                             window_secs: int, selectors: list[Selector], group_by_labels: Optional[list[str]] = None,
                             annotations: Optional[list[Annotation]] = None):
    alert = generate_alert(name, project_id, channels, window_secs, group_by_labels, annotations)
    alert.type = Threshold(selectors, predicate_rules)
    return alert


def generate_expression_alert(name: str, project_id: str, channels: list[Channel], window_secs: int,
                              program: Program, group_by_labels: Optional[list[str]] = None,
                              annotations: Optional[list[Annotation]] = None) -> Alert:
    alert = generate_alert(name, project_id, channels, window_secs, group_by_labels, annotations)
    alert.type = Expression(program)
    return alert


def get_common_part_for_written_alert(account: str, topic_paths: list[str], datacenters: list[str]) -> tuple[
        PredicateRule, list[Selector]]:
    predicate_rule = PredicateRule(threshold_type=ThresholdType.AVG, comparison=Comparison.LTE, threshold=0,
                                   target_status=Status.ALARM)
    selectors = [
        Selector(label='service', value='pqproxy_writeSession'),
        Selector(label='OriginDC', value='*'),
        Selector(label='Topic', value='total', equal=False),
        Selector(label='Producer', value='total', equal=False),
        Selector(label='host', value='|'.join(datacenters)),
    ]
    selectors.extend(get_write_common_threshold_selectors(account, topic_paths))
    return predicate_rule, selectors


def generate_messages_written_original(account: str, topic_paths: list[str], project_id: str,
                                       channels: list[Channel], datacenters: list[str]) -> Alert:
    predicate_rule, selectors = get_common_part_for_written_alert(account, topic_paths, datacenters)

    selectors.append(Selector(label='sensor', value='MessagesWrittenOriginal'))

    return generate_threshold_alert(f'{COMMON_PREFIX}-logbroker-{account}-messages-written-original', project_id,
                                    channels, [predicate_rule], DEFAULT_WINDOW_SECS, selectors,
                                    group_by_labels=['host', 'TopicPath'])


def generate_bytes_written_original(account: str, topic_paths: list[str], project_id: str, channels: list[Channel],
                                    datacenters: list[str]) -> Alert:
    predicate_rule, selectors = get_common_part_for_written_alert(account, topic_paths, datacenters)

    selectors.append(Selector(label='sensor', value='BytesWrittenOriginal'))

    return generate_threshold_alert(f'{COMMON_PREFIX}-logbroker-{account}-bytes-written-original', project_id, channels,
                                    [predicate_rule], DEFAULT_WINDOW_SECS, selectors,
                                    group_by_labels=['host', 'TopicPath'])


def generate_source_id_max_count(account: str, topic_paths: list[str], project_id: str,
                                 channels: list[Channel]) -> Alert:
    predicate_rule = PredicateRule(threshold_type=ThresholdType.AVG, comparison=Comparison.GTE, threshold=10000,
                                   target_status=Status.ALARM)

    selectors = [
        Selector(label='service', value='pqtabletAggregatedCounters'),
        Selector(label='OriginDC', value='*'),
        Selector(label='sensor', value='SourceIdMaxCount'),
    ]
    selectors.extend(get_write_common_threshold_selectors(account, topic_paths))

    return generate_threshold_alert(f'{COMMON_PREFIX}-logbroker-{account}-source-id-max-count', project_id, channels,
                                    [predicate_rule], DEFAULT_WINDOW_SECS, selectors,
                                    group_by_labels=['host', 'TopicPath'])


def generate_partition_max_write_quota_usage(account: str, topic_paths: list[str], project_id: str,
                                             channels: list[Channel]) -> Alert:
    predicate_rules = [
        PredicateRule(threshold_type=ThresholdType.AVG, comparison=Comparison.GTE, threshold=700000,
                      target_status=Status.WARN),
        PredicateRule(threshold_type=ThresholdType.AVG, comparison=Comparison.GTE, threshold=900000,
                      target_status=Status.ALARM)
    ]

    selectors = [
        Selector(label='service', value='pqtabletAggregatedCounters'),
        Selector(label='OriginDC', value='*'),
        Selector(label='sensor', value='PartitionMaxWriteQuotaUsage'),
    ]
    selectors.extend(get_write_common_threshold_selectors(account, topic_paths))

    return generate_threshold_alert(f'{COMMON_PREFIX}-logbroker-{account}-partition-max-write-quota-usage', project_id,
                                    channels, predicate_rules, DEFAULT_WINDOW_SECS, selectors,
                                    group_by_labels=['host', 'TopicPath'])


def generate_quota_consumed(account: str, project_id: str, channels: list[Channel]) -> Alert:
    program_str = '''let consumed = {consumed_selectors};
let limit = {limit_selectors};

let use_percent = avg(consumed) / max(limit) * 100;
let usage_percent_str = to_fixed(use_percent, 2) + '%';

alarm_if(use_percent > 95);
warn_if(use_percent > 80);'''

    common_selectors = [
        Selector(label='service', value='quoter_service'),
        Selector(label='resource', value='write-quota'),
        Selector(label='host', value='cluster', equal=False),
        Selector(label='quoter', value=f'/Root/PersQueue/System/Quoters/{account}'),

    ]
    common_selectors.extend(get_common_selectors())

    consumed_selectors = common_selectors.copy()
    consumed_selectors.append(Selector(label='sensor', value='QuotaConsumed'))
    limit_selectors = common_selectors
    limit_selectors.append(Selector(label='sensor', value='Limit'))

    selectors = {
        'consumed_selectors': consumed_selectors,
        'limit_selectors': limit_selectors
    }

    program = Program(program_str, selectors)

    return generate_expression_alert(f'{COMMON_PREFIX}-logbroker-{account}-quota-consumed', project_id, channels,
                                     DEFAULT_WINDOW_SECS, program, group_by_labels=['host', 'TopicPath'])


def get_common_part_for_read_alert(account: str, topic: Topic, datacenters: list[str]) -> Alert:
    predicate_rule = PredicateRule(threshold_type=ThresholdType.AVG, comparison=Comparison.LTE, threshold=0,
                                   target_status=Status.ALARM)
    selectors = [
        Selector(label='service', value='pqproxy_readSession'),
        Selector(label='OriginDC', value='*'),
        Selector(label='Client', value='total', equal=False),
        Selector(label='Topic', value='total', equal=False),
        Selector(label='Producer', value='total', equal=False),
        Selector(label='host', value='|'.join(datacenters)),
    ]
    selectors.extend(get_read_common_selectors(account, topic))
    return predicate_rule, selectors


def generate_bytes_read(account: str, topic: Topic, project_id: str, channels: list[Channel],
                        datacenters: list[str]) -> Alert:
    predicate_rule, selectors = get_common_part_for_read_alert(account, topic, datacenters)
    selectors.append(Selector(label='sensor', value='BytesRead'))

    return generate_threshold_alert(f'{COMMON_PREFIX}-logbroker-{topic_path_to_name(topic.topic_path)}-bytes-read',
                                    project_id, channels, [predicate_rule], DEFAULT_WINDOW_SECS, selectors,
                                    group_by_labels=['host', 'ConsumerPath'])


def generate_messages_read(account: str, topic: Topic, project_id: str, channels: list[Channel],
                           datacenters: list[str]) -> Alert:
    predicate_rule, selectors = get_common_part_for_read_alert(account, topic, datacenters)
    selectors.append(Selector(label='sensor', value='MessagesRead'))

    return generate_threshold_alert(f'{COMMON_PREFIX}-logbroker-{topic_path_to_name(topic.topic_path)}-messages-read',
                                    project_id, channels, [predicate_rule], DEFAULT_WINDOW_SECS, selectors,
                                    group_by_labels=['host', 'ConsumerPath'])


def generate_read_offset_rewind_sum(account: str, topic: Topic, project_id: str, channels: list[Channel]) -> Alert:
    predicate_rule = PredicateRule(threshold_type=ThresholdType.AVG, comparison=Comparison.GT, threshold=0,
                                   target_status=Status.ALARM)
    selectors = [
        Selector(label='service', value='pqtabletAggregatedCounters'),
        Selector(label='OriginDC', value='*'),
        Selector(label='sensor', value='ReadOffsetRewindSum'),
        Selector(label='Important', value='1'),
        Selector(label='user_counters', value='PersQueue'),
    ]
    selectors.extend(get_read_common_selectors(account, topic))

    return generate_threshold_alert(
        f'{COMMON_PREFIX}-logbroker-{topic_path_to_name(topic.topic_path)}-read-offset-rewind-sum', project_id,
        channels, [predicate_rule], DEFAULT_WINDOW_SECS, selectors, group_by_labels=['host', 'ConsumerPath'])


def generate_read_time_lag_ms(account: str, topic: Topic, project_id: str, channels: list[Channel],
                              datacenters: list[str]) -> Alert:
    predicate_rule = PredicateRule(threshold_type=ThresholdType.AVG, comparison=Comparison.GTE, threshold=1000000,
                                   target_status=Status.ALARM)
    selectors = [
        Selector(label='service', value='pqtabletAggregatedCounters'),
        Selector(label='OriginDC', value='*'),
        Selector(label='sensor', value='TimeSinceLastReadMs'),
        Selector(label='host', value='|'.join(datacenters)),
    ]
    selectors.extend(get_read_common_selectors(account, topic))

    return generate_threshold_alert(
        f'{COMMON_PREFIX}-logbroker-{topic_path_to_name(topic.topic_path)}-read-time-lag-ms', project_id, channels,
        [predicate_rule], DEFAULT_WINDOW_SECS, selectors, group_by_labels=['host', 'ConsumerPath'])


def generate_all_alerts(config: Config) -> list[Alert]:
    alerts = [
        generate_messages_written_original(config.account, config.topic_paths, config.solomon_project_id,
                                           config.channels, config.datacenters),

        generate_bytes_written_original(config.account, config.topic_paths, config.solomon_project_id, config.channels,
                                        config.datacenters),

        generate_source_id_max_count(config.account, config.topic_paths, config.solomon_project_id, config.channels),

        generate_partition_max_write_quota_usage(config.account, config.topic_paths, config.solomon_project_id,
                                                 config.channels),

        generate_quota_consumed(config.account, config.solomon_project_id, config.channels),
    ]

    for topic in config.topics:
        alerts.extend([
            generate_bytes_read(config.account, topic, config.solomon_project_id, config.channels, config.datacenters),

            generate_messages_read(config.account, topic, config.solomon_project_id, config.channels,
                                   config.datacenters),

            generate_read_offset_rewind_sum(config.account, topic, config.solomon_project_id, config.channels),

            generate_read_time_lag_ms(config.account, topic, config.solomon_project_id, config.channels,
                                      config.datacenters)
        ])
    return alerts
