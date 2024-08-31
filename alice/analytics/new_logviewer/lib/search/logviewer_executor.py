from yt.wrapper import YtClient
import yt.clickhouse as chyt

from datetime import (
    datetime,
    timedelta,
    timezone
)

from typing import Callable

import attr

from alice.analytics.new_logviewer.lib.search.logviewer_request import LogviewerRequest
from alice.analytics.new_logviewer.lib.search.logviewer_response import LogviewerResponse
from alice.analytics.new_logviewer.lib.search.logviewer_response import generate_logviewer_response


@attr.s
class LogviewerExecutorConfig:
    cluster: str = attr.ib()
    clique: str = attr.ib()
    storage_directory: str = attr.ib()
    date_to_table: Callable[[datetime], str] = attr.ib()
    table_granularity: timedelta = attr.ib()
    # TODO(@ran1s) delete this
    need_time_condition: bool = attr.ib()


class LogviewerExecutor:
    LIMIT_NUMBER = 500

    def __init__(self, request: LogviewerRequest, config: LogviewerExecutorConfig, yt_client: YtClient):
        self.request = request
        self.config = config
        self.yt_client = yt_client

    def execute(self) -> LogviewerResponse:
        dialog_query = self.__get_dialog_query()
        number_query = self.__get_number_query()
        row_dialog_iterator = chyt.execute(dialog_query, alias=self.config.clique, client=self.yt_client)
        row_number_iterator = chyt.execute(number_query, alias=self.config.clique, client=self.yt_client)
        return generate_logviewer_response(row_dialog_iterator, row_number_iterator)

    @staticmethod
    def inside_segment(dt: datetime, dt_left: datetime, dt_right: datetime):
        return dt_left <= dt < dt_right

    @staticmethod
    def floor_datetime(dt: datetime, granularity: timedelta):
        seconds = int(granularity.total_seconds())
        timestamp = int(dt.replace(tzinfo=timezone.utc).timestamp())
        return datetime.fromtimestamp(timestamp // seconds * seconds)

    @staticmethod
    def ceil_datetime(dt: datetime, granularity: timedelta):
        seconds = int(granularity.total_seconds())
        timestamp = int(dt.replace(tzinfo=timezone.utc).timestamp())
        return datetime.fromtimestamp((timestamp + seconds - 1) // seconds * seconds)

    def need_time_condition(self, cur: datetime, start: datetime, finish: datetime):
        return (self.inside_segment(cur, start, start + self.config.table_granularity)
                or self.inside_segment(cur, finish - self.config.table_granularity, finish))

    def __get_dialog_query(self) -> str:
        start = self.floor_datetime(datetime.fromisoformat(self.request.begin), self.config.table_granularity)
        finish = self.ceil_datetime(datetime.fromisoformat(self.request.end), self.config.table_granularity)
        queries = []
        cur = start
        while cur < finish:
            need_time_condition = self.need_time_condition(cur, start, finish) and self.config.need_time_condition
            queries.append(self.__get_dialog_table_query(self.config.date_to_table(cur), need_time_condition))
            cur += self.config.table_granularity
        select_body = "SELECT * FROM (\n"
        from_body = "\nUNION ALL\n".join(queries)
        order_body = "\n) ORDER BY `ts`\n"
        limit_body = f"LIMIT {self.LIMIT_NUMBER}"
        return select_body + from_body + order_body + limit_body

    def __get_number_query(self) -> str:
        start = self.floor_datetime(datetime.fromisoformat(self.request.begin), self.config.table_granularity)
        finish = self.ceil_datetime(datetime.fromisoformat(self.request.end), self.config.table_granularity)
        queries = []
        cur = start
        while cur < finish:
            need_time_condition = self.need_time_condition(cur, start, finish) and self.config.need_time_condition
            queries.append(self.__get_number_table_query(self.config.date_to_table(cur), need_time_condition))
            cur += self.config.table_granularity
        return "\nUNION ALL\n".join(queries)

    def __get_dialog_table_query(self, table: str, need_time_condition: bool) -> str:
        select_body = f"SELECT * FROM `{self.config.storage_directory}/{table}`\n"
        conditions = self.__get_conditions(need_time_condition)
        where_body = f"WHERE {conditions}\n" if conditions else str()
        limit_body = f"LIMIT {self.LIMIT_NUMBER}"
        return select_body + where_body + limit_body

    def __get_number_table_query(self, table: str, need_time_condition: bool) -> str:
        select_body = f"SELECT '{table}' AS `date`, COUNT(*) AS `number` FROM `{self.config.storage_directory}/{table}`"
        conditions = self.__get_conditions(need_time_condition)
        where_body = f"\nWHERE {conditions}" if conditions else str()
        return select_body + where_body

    def __get_conditions(self, need_time_condition: bool) -> str:
        equality_fields = ["uuid", "app", "skill_id"]
        regexp_fields = ["query", "reply", "intent", "generic_scenario", "mm_scenario"]
        containing_fields = ["expboxes"]

        equality = [f"`{field}` = '{getattr(self.request, field)}'"
                    for field in equality_fields
                    if getattr(self.request, field)]
        regexp = [f"match(`{field}`, '{getattr(self.request, field)}')"
                  for field in regexp_fields
                  if getattr(self.request, field)]
        containing = [f"match(`{field}`, '\\\\b{getattr(self.request, field)}\\\\b')"
                      for field in containing_fields
                      if getattr(self.request, field)]

        is_newbie = []
        if self.request.is_newbie:
            is_newbie += ["`is_new` = '1 week'"]

        time = []
        if need_time_condition:
            time += [f"(`ts` BETWEEN '{self.request.begin}' AND '{self.request.end}')"]

        conditions = []
        conditions.extend(equality)
        conditions.extend(regexp)
        conditions.extend(containing)
        conditions.extend(time)
        conditions.extend(is_newbie)

        return " AND ".join(conditions)
