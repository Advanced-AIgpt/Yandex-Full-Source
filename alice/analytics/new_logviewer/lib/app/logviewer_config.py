import re

from datetime import timedelta

from alice.analytics.new_logviewer.lib.search.logviewer_executor import LogviewerExecutorConfig
from alice.analytics.new_logviewer.lib.app.metrics import LogviewerMetricsConfig


class LogviewerConfig:
    FOLDER_PATH_PATTERN = r"^/(/|(/[\w-]+)+)$"
    TABLE_GRANULARITY = {
        "1d": {
            'date_to_table': lambda t: t.strftime("%Y-%m-%d"),
            'table_granularity': timedelta(days=1),
            'need_time_condition': True
        },
        "15min": {
            'date_to_table': lambda t: t.strftime("%Y-%m-%dT%H:%M:%S"),
            'table_granularity': timedelta(minutes=15),
            'need_time_condition': False
        }
    }

    def __init__(self, config: dict):
        self.host = config["host"]
        self.port = config["port"]
        self.storage_directory = config["storage_directory"]
        self.clique = config["clique"]
        self.cluster = config["cluster"]
        self.view = config["view"]
        self.search_solomon_buckets = config["search_solomon_buckets"]
        self.skill_solomon_buckets = config["skill_solomon_buckets"]
        self.logging_config = config["logging_config"]
        self.table_granularity = config["table_granularity"]

        self.validate_storage_directory(self.storage_directory)

    def to_logviewer_executor_config(self) -> LogviewerExecutorConfig:
        granularity_config = LogviewerConfig.TABLE_GRANULARITY[self.table_granularity]
        return LogviewerExecutorConfig(
            self.cluster,
            self.clique,
            self.storage_directory,
            granularity_config['date_to_table'],
            granularity_config['table_granularity'],
            granularity_config['need_time_condition']
        )

    def to_logviewer_metrics_config(self) -> LogviewerMetricsConfig:
        return LogviewerMetricsConfig(
            self.search_solomon_buckets,
            self.skill_solomon_buckets
        )

    @staticmethod
    def validate_storage_directory(path: str):
        if not re.match(LogviewerConfig.FOLDER_PATH_PATTERN, path):
            raise ConfigError(f"<{path}> is incorrect folder path.\n"
                              f"Path must be absolute. Example:\n"
                              f"<//home/alice-dev/andreyshspb/wonderlogs_extraction>")


class ConfigError(Exception):
    def __init__(self, message):
        super().__init__(message)
