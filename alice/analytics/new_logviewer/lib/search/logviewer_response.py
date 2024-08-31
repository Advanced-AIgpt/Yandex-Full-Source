import attr
import operator


HEADERS = ["ts", "uuid", "req_id", "query", "reply", "app", "intent",
           "generic_scenario", "mm_scenario", "is_new"]


@attr.s
class LogviewerDailyStatistic:
    date: str = attr.ib()
    number: int = attr.ib()


@attr.s
class LogviewerResponse:
    headers: list[str] = attr.ib()
    data: list[list[str]] = attr.ib()
    number: int = attr.ib()
    daily_statistics: list[LogviewerDailyStatistic] = attr.ib()


def generate_logviewer_response(row_dialog_iterator, row_number_iterator) -> LogviewerResponse:
    data = []
    for row in row_dialog_iterator:
        data.append([row.get(header) for header in HEADERS])
    total_number = 0
    daily_statistics = []
    for row in row_number_iterator:
        total_number += int(row["number"])
        daily_statistics.append(LogviewerDailyStatistic(row["date"], int(row["number"])))
    daily_statistics.sort(key=operator.attrgetter("date"))
    return LogviewerResponse(HEADERS, data, total_number, daily_statistics)
