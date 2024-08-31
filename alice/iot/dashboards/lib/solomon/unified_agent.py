from alice.iot.dashboards.lib.solomon.dashboard_builder import DashboardBuilder, Panel


class BulbasaurUnifiedAgentDashboards(DashboardBuilder):
    PROJECT = "alice-iot"
    CLUSTER = "bulbasaur_prod_unified_agent|bulbasaur_beta_unified_agent|bulbasaur_trunk_unified_agent"
    SERVICE = "unified-agent"

    PARAMETERS = [
        {
            "name": "project",
            "value": PROJECT,
        },
        {
            "name": "cluster",
            "value": CLUSTER,
        },
        {
            "name": "service",
            "value": SERVICE,
        },
        {
            "name": "host",
            "value": "*",
        },
    ]

    def __init__(self, oauth_token):
        DashboardBuilder.__init__(self, oauth_token)

    def update_dashboards(self):
        # self.update_unified_agent_solomon_metrics()
        self.update_unified_agent_health()

    def update_unified_agent_solomon_metrics(self):
        self.update(
            project_id=self.PROJECT,
            dashboard_id='bulbasaur_unified_agent_solomon_metrics',
            name='Unified Agent: device metrics delivery pipeline',
            parameters=self.PARAMETERS,
            rows=[
                [
                    Panel(
                        title="Input filter: success",
                        url=DashboardBuilder.url_query(
                            "project", self.PROJECT,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "sensor", "Inflight*|Received*|Ack*",
                            "plugin_id", "input-metrics",
                            "_flow", "*",
                            "graph", "auto",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                    Panel(
                        title="Input filter: fails",
                        url=DashboardBuilder.url_query(
                            "project", self.PROJECT,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "sensor", "Backpressure|Dropped*|HollowMessages",
                            "plugin_id", "input-metrics",
                            "graph", "auto",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                    Panel(
                        title="Input filter: response timings (millis)",
                        url=DashboardBuilder.url_query(
                            "project", self.PROJECT,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "sensor", "ResponseTimeMillis",
                            "plugin_id", "input-metrics",
                            "overLinesTransform", "WEIGHTED_PERCENTILE",
                            "percentiles", "95,99",
                            "graph", "auto",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                ],
                [
                    Panel(
                        title="Storage filter: success",
                        url=DashboardBuilder.url_query(
                            "project", self.PROJECT,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "sensor", "Ack*|Inflight*",
                            "plugin_id", "storage-history_metrics_storage",
                            "graph", "auto",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                    Panel(
                        title="Storage filter: fails",
                        url=DashboardBuilder.url_query(
                            "project", self.PROJECT,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "sensor", "Backpressure|Dropped*|HollowMessages",
                            "plugin_id", "storage-history_metrics_storage",
                            "graph", "auto",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                    Panel(
                        title="Storage filter: stored",
                        url=DashboardBuilder.url_query(
                            "project", self.PROJECT,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "sensor", "Stored*",
                            "plugin_id", "storage-history_metrics_storage",
                            "graph", "auto",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    )
                ],
                [
                    Panel(
                        title="Accumulate filter: success",
                        url=DashboardBuilder.url_query(
                            "project", self.PROJECT,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "sensor", "ProcessedMessages|ProducedMetrics",
                            "plugin_id", "filter-accumulate_metrics",
                            "graph", "auto",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                    Panel(
                        title="Accumulate filter: fails",
                        url=DashboardBuilder.url_query(
                            "project", self.PROJECT,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "sensor", "DroppedMessages",
                            "plugin_id", "filter-accumulate_metrics",
                            "graph", "auto",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                    Panel(
                        title="Storage filter: trailing",
                        url=DashboardBuilder.url_query(
                            "project", self.PROJECT,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "sensor", "Trailing*",
                            "plugin_id", "storage-history_metrics_storage",
                            "graph", "auto",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                ],
                [
                    Panel(
                        title="Output filter: success",
                        url=DashboardBuilder.url_query(
                            "project", self.PROJECT,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "sensor", "PipelineInflight*|Ack*|Plugin*|Received*",
                            "plugin_id", "output-metrics",
                            "graph", "auto",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                    Panel(
                        title="Output filter: fails",
                        url=DashboardBuilder.url_query(
                            "project", self.PROJECT,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "sensor", "DroppedMessages|FailedAttempts|HollowMessages|",
                            "plugin_id", "output-metrics",
                            "_flow", "*",
                            "graph", "auto",
                            "legend", "off"
                        ),
                        rowspan=1,
                        colspan=1,
                    ),
                ]
            ]
        )

    def update_unified_agent_health(self):
        self.update(
            project_id=self.PROJECT,
            dashboard_id='bulbasaur_unified_agent_health',
            name='Unified Agent: health',
            parameters=self.PARAMETERS,
            rows=[
                [
                    Panel(
                        title="Uptime",
                        url=DashboardBuilder.url_query(
                            "project", self.PROJECT,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "sensor", "Uptime",
                            "scope", "system",
                            "graph", "auto",
                            "legend", "off"
                        ),
                    ),
                    Panel(
                        title="Health",
                        url=DashboardBuilder.url_query(
                            "project", self.PROJECT,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "scope", "health",
                            "graph", "auto",
                            "legend", "off"
                        ),
                    )
                ],
                [
                    Panel(
                        title="CPU",
                        url=DashboardBuilder.url_query(
                            "project", self.PROJECT,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "scope", "system",
                            "sensor", "Cpu*",
                            "graph", "auto",
                            "legend", "off"
                        ),
                    ),
                    Panel(
                        title="Memory (bytes)",
                        url=DashboardBuilder.url_query(
                            "project", self.PROJECT,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "scope", "system",
                            "sensor", "Vsize|RSS",
                            "graph", "auto",
                            "legend", "off"
                        ),
                    )
                ],
                [
                    Panel(
                        title="Threads",
                        url=DashboardBuilder.url_query(
                            "project", self.PROJECT,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "scope", "system",
                            "sensor", "NumThreads",
                            "graph", "auto",
                            "legend", "off"
                        ),
                    ),
                    Panel(
                        title="Page faults",
                        url=DashboardBuilder.url_query(
                            "project", self.PROJECT,
                            "cluster", "{{cluster}}",
                            "service", "{{service}}",
                            "host", "{{host}}",
                            "scope", "system",
                            "sensor", "MajorPageFaults|MinorPageFaults",
                            "graph", "auto",
                            "legend", "off"
                        ),
                    ),
                ],
            ],
        )
