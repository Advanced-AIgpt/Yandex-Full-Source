import pytest
from alice.wonderlogs.tools.lb_alert_creator.lib.alerts import (
    generate_all_alerts,
    topic_path_to_name,
)
from alice.wonderlogs.tools.lb_alert_creator.lib.config import Config
from alice.wonderlogs.tools.lb_alert_creator.lib.entities import Status, Channel, Topic


@pytest.fixture()
def config():
    channels = [
        Channel(id='ran1s_tg', notify_about_statuses=[Status.ALARM, Status.WARN], repeat_delay_secs=300),
    ]
    topics = [
        Topic(topic_path='megamind/analytics-log', consumer_paths=['shared/hahn-logfeller-shadow']),
        Topic(topic_path='megamind/proactivity-log',
              consumer_paths=['rtmr/rtmr-man-prestable', 'rtmr/rtmr-sas', 'shared/hahn-logfeller-shadow',
                              'shared/rtmr-vla']),
        Topic(topic_path='megamind/apphost/prod/event-log',
              consumer_paths=['alicelogs/prod/rthub', 'shared/hahn-logfeller-shadow']),
    ]
    return Config(account='megamind',
                  topics=topics,
                  channels=channels,
                  datacenters=['Sas', 'Man', 'Vla'],
                  solomon_project_id='megamind')


def test_generate_all_alerts(config):
    expected = [{
        'id': 'd401d9de-d60a-83cf-4d4b-473dda3b0f97',
        'projectId': 'megamind',
        'name': 'autogen-logbroker-megamind-messages-written-original',
        'channels': [
            {
                'id': 'ran1s_tg',
                'config': {
                    'notifyAboutStatuses': ['ALARM', 'WARN'],
                    'repeatDelaySecs': 300,
                },
            }
        ],
        'type': {
            'threshold': {
                'selectors': (
                    "{service='pqproxy_writeSession', OriginDC='*', Topic!='total', Producer!='total', "
                    "host='Sas|Man|Vla', cluster='lbk', project='kikimr', Account='megamind', "
                    "TopicPath='megamind/analytics-log|megamind/proactivity-log|megamind/apphost/prod/event-log', "
                    "sensor='MessagesWrittenOriginal'}"),
                'predicateRules': [
                    {
                        'thresholdType': 'AVG',
                        'comparison': 'LTE',
                        'threshold': 0,
                        'targetStatus': 'ALARM',
                    }
                ],
            }
        },
        'windowSecs': 300,
        'delaySecs': 0,
        'groupByLabels': ['host', 'TopicPath'],
    }, {
        'id': '66c2f890-0854-8dea-661e-9062f5b25653',
        'projectId': 'megamind',
        'name': 'autogen-logbroker-megamind-bytes-written-original',
        'channels': [
            {
                'id': 'ran1s_tg',
                'config': {
                    'notifyAboutStatuses': ['ALARM', 'WARN'],
                    'repeatDelaySecs': 300,
                },
            }
        ],
        'type': {
            'threshold': {
                'selectors': (
                    "{service='pqproxy_writeSession', OriginDC='*', Topic!='total', Producer!='total', "
                    "host='Sas|Man|Vla', cluster='lbk', project='kikimr', Account='megamind', "
                    "TopicPath='megamind/analytics-log|megamind/proactivity-log|megamind/apphost/prod/event-log', "
                    "sensor='BytesWrittenOriginal'}"),
                'predicateRules': [
                    {
                        'thresholdType': 'AVG',
                        'comparison': 'LTE',
                        'threshold': 0,
                        'targetStatus': 'ALARM',
                    }
                ],
            }
        },
        'windowSecs': 300,
        'delaySecs': 0,
        'groupByLabels': ['host', 'TopicPath'],
    }, {
        'id': 'e023cb50-d8a6-878c-950b-b3b36c789165',
        'projectId': 'megamind',
        'name': 'autogen-logbroker-megamind-source-id-max-count',
        'channels': [
            {
                'id': 'ran1s_tg',
                'config': {
                    'notifyAboutStatuses': ['ALARM', 'WARN'],
                    'repeatDelaySecs': 300,
                },
            }
        ],
        'type': {
            'threshold': {
                'selectors': ("{service='pqtabletAggregatedCounters', OriginDC='*', sensor='SourceIdMaxCount', "
                              "cluster='lbk', project='kikimr', Account='megamind', "
                              "TopicPath='megamind/analytics-log|megamind/proactivity-log|"
                              "megamind/apphost/prod/event-log'}"),
                'predicateRules': [
                    {
                        'thresholdType': 'AVG',
                        'comparison': 'GTE',
                        'threshold': 10000,
                        'targetStatus': 'ALARM',
                    }
                ],
            }
        },
        'windowSecs': 300,
        'delaySecs': 0,
        'groupByLabels': ['host', 'TopicPath'],
    }, {
        'id': '068e1304-84c4-afbb-72e5-1996dde94825',
        'projectId': 'megamind',
        'name': 'autogen-logbroker-megamind-partition-max-write-quota-usage',
        'channels': [
            {
                'id': 'ran1s_tg',
                'config': {
                    'notifyAboutStatuses': ['ALARM', 'WARN'],
                    'repeatDelaySecs': 300,
                },
            }
        ],
        'type': {
            'threshold': {
                'selectors': "{service='pqtabletAggregatedCounters', OriginDC='*', "
                             "sensor='PartitionMaxWriteQuotaUsage', cluster='lbk', project='kikimr', "
                             "Account='megamind', TopicPath='megamind/analytics-log|megamind/proactivity-log|"
                             "megamind/apphost/prod/event-log'}",
                'predicateRules': [
                    {
                        'thresholdType': 'AVG',
                        'comparison': 'GTE',
                        'threshold': 700000,
                        'targetStatus': 'WARN',
                    },
                    {
                        'thresholdType': 'AVG',
                        'comparison': 'GTE',
                        'threshold': 900000,
                        'targetStatus': 'ALARM',
                    },
                ],
            }
        },
        'windowSecs': 300,
        'delaySecs': 0,
        'groupByLabels': ['host', 'TopicPath'],
    }, {
        'id': 'a65385d6-b9ce-ec93-9095-33261d44f5d0',
        'projectId': 'megamind',
        'name': 'autogen-logbroker-megamind-quota-consumed',
        'channels': [
            {
                'id': 'ran1s_tg',
                'config': {
                    'notifyAboutStatuses': ['ALARM', 'WARN'],
                    'repeatDelaySecs': 300,
                },
            }
        ],
        'type': {
            'expression': {
                'program': ("let consumed = "
                            "{service='quoter_service', resource='write-quota', host!='cluster', "
                            "quoter='/Root/PersQueue/System/Quoters/megamind', cluster='lbk', project='kikimr', "
                            "sensor='QuotaConsumed'};\n"

                            "let limit = {service='quoter_service', resource='write-quota', host!='cluster', "
                            "quoter='/Root/PersQueue/System/Quoters/megamind', cluster='lbk', project='kikimr', "
                            "sensor='Limit'};\n\n"

                            "let use_percent = avg(consumed) / max(limit) * 100;\n"
                            "let usage_percent_str = to_fixed(use_percent, 2) + '%';\n\n"

                            "alarm_if(use_percent > 95);\n"
                            "warn_if(use_percent > 80);"),
                'checkExpression': '',
            }
        },
        'windowSecs': 300,
        'delaySecs': 0,
        'groupByLabels': ['host', 'TopicPath'],
    }, {
        'id': '0d9852fe-66ad-6b97-86ff-bcc4cc5a1292',
        'projectId': 'megamind',
        'name': 'autogen-logbroker-megamind-analytics-log-bytes-read',
        'channels': [
            {
                'id': 'ran1s_tg',
                'config': {
                    'notifyAboutStatuses': ['ALARM', 'WARN'],
                    'repeatDelaySecs': 300,
                },
            }
        ],
        'type': {
            'threshold': {
                'selectors': ("{service='pqproxy_readSession', OriginDC='*', Client!='total', Topic!='total', "
                              "Producer!='total', host='Sas|Man|Vla', cluster='lbk', project='kikimr', "
                              "Account='megamind', TopicPath='megamind/analytics-log', "
                              "ConsumerPath='shared/hahn-logfeller-shadow', sensor='BytesRead'}"),
                'predicateRules': [
                    {
                        'thresholdType': 'AVG',
                        'comparison': 'LTE',
                        'threshold': 0,
                        'targetStatus': 'ALARM',
                    }
                ],
            }
        },
        'windowSecs': 300,
        'delaySecs': 0,
        'groupByLabels': ['host', 'ConsumerPath'],
    }, {
        'id': '6338d36b-a54a-fb07-b52f-a17de77e900d',
        'projectId': 'megamind',
        'name': 'autogen-logbroker-megamind-analytics-log-messages-read',
        'channels': [
            {
                'id': 'ran1s_tg',
                'config': {
                    'notifyAboutStatuses': ['ALARM', 'WARN'],
                    'repeatDelaySecs': 300,
                },
            }
        ],
        'type': {
            'threshold': {
                'selectors': ("{service='pqproxy_readSession', OriginDC='*', Client!='total', Topic!='total', "
                              "Producer!='total', host='Sas|Man|Vla', cluster='lbk', project='kikimr', "
                              "Account='megamind', TopicPath='megamind/analytics-log', "
                              "ConsumerPath='shared/hahn-logfeller-shadow', sensor='MessagesRead'}"),
                'predicateRules': [
                    {
                        'thresholdType': 'AVG',
                        'comparison': 'LTE',
                        'threshold': 0,
                        'targetStatus': 'ALARM',
                    }
                ],
            }
        },
        'windowSecs': 300,
        'delaySecs': 0,
        'groupByLabels': ['host', 'ConsumerPath'],
    }, {
        'id': 'c53bdca4-fac7-cc52-e9a1-d6803b84c0d4',
        'projectId': 'megamind',
        'name': 'autogen-logbroker-megamind-analytics-log-read-offset-rewind-sum',
        'channels': [
            {
                'id': 'ran1s_tg',
                'config': {
                    'notifyAboutStatuses': ['ALARM', 'WARN'],
                    'repeatDelaySecs': 300,
                },
            }
        ],
        'type': {
            'threshold': {
                'selectors': ("{service='pqtabletAggregatedCounters', OriginDC='*', sensor='ReadOffsetRewindSum', "
                              "Important='1', user_counters='PersQueue', cluster='lbk', project='kikimr', "
                              "Account='megamind', TopicPath='megamind/analytics-log', "
                              "ConsumerPath='shared/hahn-logfeller-shadow'}"),
                'predicateRules': [
                    {
                        'thresholdType': 'AVG',
                        'comparison': 'GT',
                        'threshold': 0,
                        'targetStatus': 'ALARM',
                    }
                ],
            }
        },
        'windowSecs': 300,
        'delaySecs': 0,
        'groupByLabels': ['host', 'ConsumerPath'],
    }, {
        'id': 'abadd5ff-704e-3438-ebfb-ea05e04bc480',
        'projectId': 'megamind',
        'name': 'autogen-logbroker-megamind-analytics-log-read-time-lag-ms',
        'channels': [
            {
                'id': 'ran1s_tg',
                'config': {
                    'notifyAboutStatuses': ['ALARM', 'WARN'],
                    'repeatDelaySecs': 300,
                },
            }
        ],
        'type': {
            'threshold': {
                'selectors': ("{service='pqtabletAggregatedCounters', OriginDC='*', sensor='TimeSinceLastReadMs', "
                              "host='Sas|Man|Vla', cluster='lbk', project='kikimr', Account='megamind', "
                              "TopicPath='megamind/analytics-log', ConsumerPath='shared/hahn-logfeller-shadow'}"),
                'predicateRules': [
                    {
                        'thresholdType': 'AVG',
                        'comparison': 'GTE',
                        'threshold': 1000000,
                        'targetStatus': 'ALARM',
                    }
                ],
            }
        },
        'windowSecs': 300,
        'delaySecs': 0,
        'groupByLabels': ['host', 'ConsumerPath'],
    }, {
        'id': '14a17108-4011-4a6d-232f-3964adc9dcca',
        'projectId': 'megamind',
        'name': 'autogen-logbroker-megamind-proactivity-log-bytes-read',
        'channels': [
            {
                'id': 'ran1s_tg',
                'config': {
                    'notifyAboutStatuses': ['ALARM', 'WARN'],
                    'repeatDelaySecs': 300,
                },
            }
        ],
        'type': {
            'threshold': {
                'selectors': ("{service='pqproxy_readSession', OriginDC='*', Client!='total', Topic!='total', "
                              "Producer!='total', host='Sas|Man|Vla', cluster='lbk', project='kikimr', "
                              "Account='megamind', TopicPath='megamind/proactivity-log', "
                              "ConsumerPath='rtmr/rtmr-man-prestable|rtmr/rtmr-sas|shared/hahn-logfeller-shadow|"
                              "shared/rtmr-vla', sensor='BytesRead'}"),
                'predicateRules': [
                    {
                        'thresholdType': 'AVG',
                        'comparison': 'LTE',
                        'threshold': 0,
                        'targetStatus': 'ALARM',
                    }
                ],
            }
        },
        'windowSecs': 300,
        'delaySecs': 0,
        'groupByLabels': ['host', 'ConsumerPath'],
    }, {
        'id': 'fcd1f1f9-f418-14d0-17cd-2198c7c250aa',
        'projectId': 'megamind',
        'name': 'autogen-logbroker-megamind-proactivity-log-messages-read',
        'channels': [
            {
                'id': 'ran1s_tg',
                'config': {
                    'notifyAboutStatuses': ['ALARM', 'WARN'],
                    'repeatDelaySecs': 300,
                },
            }
        ],
        'type': {
            'threshold': {
                'selectors': ("{service='pqproxy_readSession', OriginDC='*', Client!='total', Topic!='total', "
                              "Producer!='total', host='Sas|Man|Vla', cluster='lbk', project='kikimr', "
                              "Account='megamind', TopicPath='megamind/proactivity-log', "
                              "ConsumerPath='rtmr/rtmr-man-prestable|rtmr/rtmr-sas|shared/hahn-logfeller-shadow|"
                              "shared/rtmr-vla', sensor='MessagesRead'}"),
                'predicateRules': [
                    {
                        'thresholdType': 'AVG',
                        'comparison': 'LTE',
                        'threshold': 0,
                        'targetStatus': 'ALARM',
                    }
                ],
            }
        },
        'windowSecs': 300,
        'delaySecs': 0,
        'groupByLabels': ['host', 'ConsumerPath'],
    }, {
        'id': '482a2ac1-4bd8-4fb2-a2ab-5147e6ed71e7',
        'projectId': 'megamind',
        'name': 'autogen-logbroker-megamind-proactivity-log-read-offset-rewind-sum',
        'channels': [
            {
                'id': 'ran1s_tg',
                'config': {
                    'notifyAboutStatuses': ['ALARM', 'WARN'],
                    'repeatDelaySecs': 300,
                },
            }
        ],
        'type': {
            'threshold': {
                'selectors': ("{service='pqtabletAggregatedCounters', OriginDC='*', sensor='ReadOffsetRewindSum', "
                              "Important='1', user_counters='PersQueue', cluster='lbk', project='kikimr', "
                              "Account='megamind', TopicPath='megamind/proactivity-log', "
                              "ConsumerPath='rtmr/rtmr-man-prestable|rtmr/rtmr-sas|shared/hahn-logfeller-shadow|"
                              "shared/rtmr-vla'}"),
                'predicateRules': [
                    {
                        'thresholdType': 'AVG',
                        'comparison': 'GT',
                        'threshold': 0,
                        'targetStatus': 'ALARM',
                    }
                ],
            }
        },
        'windowSecs': 300,
        'delaySecs': 0,
        'groupByLabels': ['host', 'ConsumerPath'],
    }, {
        'id': 'ff6c9d3b-9d41-b633-5e17-f759f5511783',
        'projectId': 'megamind',
        'name': 'autogen-logbroker-megamind-proactivity-log-read-time-lag-ms',
        'channels': [
            {
                'id': 'ran1s_tg',
                'config': {
                    'notifyAboutStatuses': ['ALARM', 'WARN'],
                    'repeatDelaySecs': 300,
                },
            }
        ],
        'type': {
            'threshold': {
                'selectors': ("{service='pqtabletAggregatedCounters', OriginDC='*', sensor='TimeSinceLastReadMs', "
                              "host='Sas|Man|Vla', cluster='lbk', project='kikimr', Account='megamind', "
                              "TopicPath='megamind/proactivity-log', ConsumerPath='rtmr/rtmr-man-prestable|"
                              "rtmr/rtmr-sas|shared/hahn-logfeller-shadow|shared/rtmr-vla'}"),
                'predicateRules': [
                    {
                        'thresholdType': 'AVG',
                        'comparison': 'GTE',
                        'threshold': 1000000,
                        'targetStatus': 'ALARM',
                    }
                ],
            }
        },
        'windowSecs': 300,
        'delaySecs': 0,
        'groupByLabels': ['host', 'ConsumerPath'],
    }, {
        'id': '92a8d9fa-ae9e-22c2-bf4b-6dfe33a2b7ad',
        'projectId': 'megamind',
        'name': 'autogen-logbroker-megamind-apphost-prod-event-log-bytes-read',
        'channels': [
            {
                'id': 'ran1s_tg',
                'config': {
                    'notifyAboutStatuses': ['ALARM', 'WARN'],
                    'repeatDelaySecs': 300,
                },
            }
        ],
        'type': {
            'threshold': {
                'selectors': ("{service='pqproxy_readSession', OriginDC='*', Client!='total', Topic!='total', "
                              "Producer!='total', host='Sas|Man|Vla', cluster='lbk', project='kikimr', "
                              "Account='megamind', TopicPath='megamind/apphost/prod/event-log', "
                              "ConsumerPath='alicelogs/prod/rthub|shared/hahn-logfeller-shadow', sensor='BytesRead'}"),
                'predicateRules': [
                    {
                        'thresholdType': 'AVG',
                        'comparison': 'LTE',
                        'threshold': 0,
                        'targetStatus': 'ALARM',
                    }
                ],
            }
        },
        'windowSecs': 300,
        'delaySecs': 0,
        'groupByLabels': ['host', 'ConsumerPath'],
    }, {
        'id': 'aa1f2416-f9b1-ad2d-1b80-1790fbf5b695',
        'projectId': 'megamind',
        'name': 'autogen-logbroker-megamind-apphost-prod-event-log-messages-read',
        'channels': [
            {
                'id': 'ran1s_tg',
                'config': {
                    'notifyAboutStatuses': ['ALARM', 'WARN'],
                    'repeatDelaySecs': 300,
                },
            }
        ],
        'type': {
            'threshold': {
                'selectors': ("{service='pqproxy_readSession', OriginDC='*', Client!='total', Topic!='total', "
                              "Producer!='total', host='Sas|Man|Vla', cluster='lbk', project='kikimr', "
                              "Account='megamind', TopicPath='megamind/apphost/prod/event-log', "
                              "ConsumerPath='alicelogs/prod/rthub|shared/hahn-logfeller-shadow', "
                              "sensor='MessagesRead'}"),
                'predicateRules': [
                    {
                        'thresholdType': 'AVG',
                        'comparison': 'LTE',
                        'threshold': 0,
                        'targetStatus': 'ALARM',
                    }
                ],
            }
        },
        'windowSecs': 300,
        'delaySecs': 0,
        'groupByLabels': ['host', 'ConsumerPath'],
    }, {
        'id': 'bab53759-730b-6ca7-b1a9-5ddc6cf10f47',
        'projectId': 'megamind',
        'name': 'autogen-logbroker-megamind-apphost-prod-event-log-read-offset-rewind-sum',
        'channels': [
            {
                'id': 'ran1s_tg',
                'config': {
                    'notifyAboutStatuses': ['ALARM', 'WARN'],
                    'repeatDelaySecs': 300,
                },
            }
        ],
        'type': {
            'threshold': {
                'selectors': ("{service='pqtabletAggregatedCounters', OriginDC='*', sensor='ReadOffsetRewindSum', "
                              "Important='1', user_counters='PersQueue', cluster='lbk', project='kikimr', "
                              "Account='megamind', TopicPath='megamind/apphost/prod/event-log', "
                              "ConsumerPath='alicelogs/prod/rthub|shared/hahn-logfeller-shadow'}"),
                'predicateRules': [
                    {
                        'thresholdType': 'AVG',
                        'comparison': 'GT',
                        'threshold': 0,
                        'targetStatus': 'ALARM',
                    }
                ],
            }
        },
        'windowSecs': 300,
        'delaySecs': 0,
        'groupByLabels': ['host', 'ConsumerPath'],
    }, {
        'id': '65fc54f3-cfbf-00b5-73fd-f75992402a08',
        'projectId': 'megamind',
        'name': 'autogen-logbroker-megamind-apphost-prod-event-log-read-time-lag-ms',
        'channels': [
            {
                'id': 'ran1s_tg',
                'config': {
                    'notifyAboutStatuses': ['ALARM', 'WARN'],
                    'repeatDelaySecs': 300,
                },
            }
        ],
        'type': {
            'threshold': {
                'selectors': ("{service='pqtabletAggregatedCounters', OriginDC='*', sensor='TimeSinceLastReadMs', "
                              "host='Sas|Man|Vla', cluster='lbk', project='kikimr', Account='megamind', "
                              "TopicPath='megamind/apphost/prod/event-log', "
                              "ConsumerPath='alicelogs/prod/rthub|shared/hahn-logfeller-shadow'}"),
                'predicateRules': [
                    {
                        'thresholdType': 'AVG',
                        'comparison': 'GTE',
                        'threshold': 1000000,
                        'targetStatus': 'ALARM',
                    }
                ],
            }
        },
        'windowSecs': 300,
        'delaySecs': 0,
        'groupByLabels': ['host', 'ConsumerPath'],
    }]

    actual = [a.to_solomon_dict() for a in generate_all_alerts(config)]

    assert expected == actual


def test_topic_path_to_name():
    assert 'megamind-proactivity-log' == topic_path_to_name('megamind/proactivity-log')
    assert 'megamind-apphost-prod-event-log' == topic_path_to_name('megamind/apphost/prod/event-log')
