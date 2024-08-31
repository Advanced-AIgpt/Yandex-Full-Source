from alice.wonderlogs.tools.lb_alert_creator.lib.entities import Channel, Topic
from dataclasses import dataclass, field


@dataclass(init=True)
class Config:
    account: str
    topics: list[Topic]
    datacenters: list[str]
    solomon_project_id: str
    channels: list[Channel]
    topic_paths: list[str] = field(init=False)

    @classmethod
    def from_dict(cls, d):
        return cls(account=d['account'], topics=[Topic.from_dict(topic) for topic in d['topics']],
                   datacenters=d['datacenters'], solomon_project_id=d['solomon_project_id'],
                   channels=list(map(Channel.from_dict, d['channels'])))

    def __post_init__(self):
        self.topic_paths = list(map(lambda t: t.topic_path, self.topics))
