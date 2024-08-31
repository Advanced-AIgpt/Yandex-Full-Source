import argparse
from typing import Optional, List
from datetime import datetime, timedelta

import voicetech.common.lib.utils as utils

import yt.wrapper as yt

logger = utils.initialize_logging(__name__)

FORMAT = '%Y%m%dT%H%M%S'


def _parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--minutes', type=float, default=3)
    parser.add_argument('--min', type=int, default=1)
    parser.add_argument('--max', type=int, default=5)
    parser.add_argument('requests_table', help='//path/to/yt/table/with/all/requests')
    parser.add_argument('session_table', help='//path/to/yt/table/with/sessions')
    parser.add_argument('message_ids', help='//path/to/yt/table/with/selected/requests')
    parser.add_argument('context_mapping', help='//path/to/yt/table/with/context_mapping')
    return parser.parse_args()


@yt.yt_dataclass
class RequestRow:
    request_id: str


@yt.yt_dataclass
class SessionsRow:
    device_id: Optional[str]
    _message_id: Optional[str]
    client_time: Optional[str]


@yt.yt_dataclass
class ResultRow:
    _message_id: str
    is_target: bool


@yt.yt_dataclass
class ContextResultRow:
    _message_id: str
    context_message_ids: List[str]


class Reducer(yt.TypedJob):
    def __init__(self, interesting_message_ids, min_context, max_context, max_minutes):
        self._interesting_message_ids = interesting_message_ids
        self._min_context = min_context
        self._max_context = max_context
        self._max_time_interval = timedelta(minutes=max_minutes)

    def prepare_operation(self, context, preparer):
        preparer.input(0, type=SessionsRow).output(0, type=ResultRow).output(1, type=ContextResultRow)

    def _make_result(self, row, used):
        if row._message_id in used:
            return
        used.add(row._message_id)
        yield yt.OutputRow(
            ResultRow(_message_id=row._message_id, is_target=row._message_id in self._interesting_message_ids),
            table_index=0,
        )

    def __call__(self, rows):
        rows = list(rows)
        used = set()
        for i, row in enumerate(rows):
            if row._message_id not in self._interesting_message_ids:
                continue
            context_row = ContextResultRow(_message_id=row._message_id, context_message_ids=[])

            for j in range(1, self._min_context + 1):
                if i - j >= 0:
                    yield from self._make_result(rows[i - j], used)
                    context_row.context_message_ids.append(rows[i - j]._message_id)
                if i + j < len(rows):
                    yield from self._make_result(rows[i + j], used)
                    context_row.context_message_ids.append(rows[i + j]._message_id)

            this_time = datetime.strptime(row.client_time, FORMAT)
            from_time = this_time - self._max_time_interval
            to_time = this_time + self._max_time_interval
            for j in range(self._min_context + 1, self._max_context + 1):
                if i - j >= 0 and datetime.strptime(rows[i - j].client_time, FORMAT) >= from_time:
                    yield from self._make_result(rows[i - j], used)
                    context_row.context_message_ids.append(rows[i - j]._message_id)
                if i + j < len(rows) and datetime.strptime(rows[i + j].client_time, FORMAT) <= to_time:
                    yield from self._make_result(rows[i + j], used)
                    context_row.context_message_ids.append(rows[i + j]._message_id)

            yield from self._make_result(rows[i], used)
            yield yt.OutputRow(context_row, table_index=1)


def main():
    args = _parse_args()

    with yt.Transaction():
        interesting_message_ids = set([r.request_id for r in yt.read_table_structured(args.requests_table, RequestRow)])
        logger.info("Found %d interesting requests", len(interesting_message_ids))

        yt.run_reduce(
            Reducer(interesting_message_ids, args.min, args.max, args.minutes),
            args.session_table,
            [args.message_ids, args.context_mapping],
            reduce_by=['device_id'],
        )
