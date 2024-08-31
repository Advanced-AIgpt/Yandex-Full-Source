from typing import Optional, Union

from dataclasses import dataclass
from enum import Enum


class ThresholdType(Enum):
    AVG = 'AVG'
    MAX = 'MAX'


class Comparison(Enum):
    LTE = 'LTE'
    GTE = 'GTE'
    GT = 'GT'


class Status(Enum):
    ALARM = 'ALARM'
    ERROR = 'ERROR'
    NO_DATA = 'NO_DATA'
    WARN = 'WARN'


@dataclass(init=True)
class PredicateRule:
    threshold_type: ThresholdType
    comparison: Comparison
    threshold: float
    target_status: Status

    def to_solomon_dict(self):
        return {
            'thresholdType': self.threshold_type.value,
            'comparison': self.comparison.value,
            'threshold': self.threshold,
            'targetStatus': self.target_status.value
        }


@dataclass(init=True)
class Selector:
    label: str
    value: str
    equal: bool = True

    def __str__(self):
        eq_or_not_eq = '=' if self.equal else '!='
        return f"{self.label}{eq_or_not_eq}'{self.value}'"


@dataclass(init=True)
class Program:
    text: str
    selectors: dict[str, list[Selector]]

    @staticmethod
    def _selectors_to_str(selectors: list[Selector]):
        selectors_str = ', '.join(map(str, selectors))
        return f'{{{selectors_str}}}'

    def __str__(self):
        selectors_str = {k: self._selectors_to_str(v) for k, v in self.selectors.items()}
        return self.text.format(**selectors_str)


@dataclass(init=True)
class Channel:
    id: str
    notify_about_statuses: list[Status]
    repeat_delay_secs: int

    @classmethod
    def from_dict(cls, d):
        return cls(id=d['id'], notify_about_statuses=list(map(Status, d['notify_about_statuses'])),
                   repeat_delay_secs=d['repeat_delay_secs'])

    def to_solomon_dict(self):
        return {
            'id': self.id,
            'config': {
                'notifyAboutStatuses': [status.value for status in self.notify_about_statuses],
                'repeatDelaySecs': self.repeat_delay_secs
            }
        }


@dataclass(init=True)
class Threshold:
    selectors: list[Selector]
    predicate_rules: list[PredicateRule]

    def to_solomon_dict(self):
        selectors_str = ', '.join(map(str, self.selectors))
        threshold = {
            'threshold': {
                'selectors': f'{{{selectors_str}}}',
                'predicateRules': []
            }
        }
        for predicate_rule in self.predicate_rules:
            threshold['threshold']['predicateRules'].append({
                'thresholdType': predicate_rule.threshold_type.value,
                'comparison': predicate_rule.comparison.value,
                'threshold': predicate_rule.threshold,
                'targetStatus': predicate_rule.target_status.value
            })
        return threshold


@dataclass(init=True)
class Annotation:
    key: str
    value: str


@dataclass(init=True)
class Expression:
    program: Program
    check_expression: str = ''

    def to_solomon_dict(self):
        return {
            'expression': {
                'program': str(self.program),
                'checkExpression': self.check_expression
            }
        }


@dataclass
class Alert:
    id: str
    project_id: str
    name: str
    version: Optional[int]
    channels: list[Channel]
    type: Union[Threshold, Expression]
    window_secs: int
    group_by_labels: Optional[list[str]]
    delay_secs: int
    annotations: list[Annotation]

    def to_solomon_dict(self):
        return {
            'id': self.id,
            'projectId': self.project_id,
            'name': self.name,
            'channels': [c.to_solomon_dict() for c in self.channels],
            'type': self.type.to_solomon_dict(),
            'windowSecs': self.window_secs,
            'delaySecs': 0,
            'groupByLabels': self.group_by_labels
        }


@dataclass(init=True)
class Topic:
    topic_path: str
    consumer_paths: list[str]

    @classmethod
    def from_dict(cls, d):
        return cls(topic_path=d['topic_path'], consumer_paths=d['consumer_paths'])
