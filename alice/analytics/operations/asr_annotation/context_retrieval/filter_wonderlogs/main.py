import argparse
import datetime as dt

import voicetech.common.lib.utils as utils

import yt.wrapper as yt

logger = utils.initialize_logging(__name__)

FORMAT = '%Y%m%dT%H%M%S'
PATH_TO_LOGS = "//home/alice/wonder/logs"


def _get_date_range(start_date, end_date):
    dates = []
    start_date = dt.datetime.strptime(start_date, '%Y-%m-%d').date()
    end_date = dt.datetime.strptime(end_date, '%Y-%m-%d').date()
    while True:
        if start_date > end_date:
            break
        dates.append(start_date)
        start_date += dt.timedelta(days=1)
    return [PATH_TO_LOGS + '/' + d.strftime('%Y-%m-%d') for d in dates]


def _parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('requests_table', help='//path/to/requests/table')
    parser.add_argument('start_date')
    parser.add_argument('end_date')
    parser.add_argument('dst', help='//path/to/result')
    return parser.parse_args()


@yt.yt_dataclass
class RequestRow:
    _message_id: str


class Mapper:
    def __init__(self, interesting_message_ids):
        self._interesting_message_ids = interesting_message_ids

    def __call__(self, row):
        if row['_message_id'] in self._interesting_message_ids:
            yield row


def main():
    args = _parse_args()
    src_list = _get_date_range(args.start_date, args.end_date)

    with yt.Transaction():
        interesting_message_ids = set(
            [r._message_id for r in yt.read_table_structured(args.requests_table, RequestRow)]
        )
        logger.info("Found %d interesting requests", len(interesting_message_ids))

        schema = yt.get(src_list[0] + '/@schema')
        new_schema = []
        for entry in schema:
            if 'sort_order' in entry:
                logger.warning('Removing sort order from column %s', entry['name'])
                entry.pop('sort_order')
            new_schema.append(entry)
        schema = new_schema
        yt.create('table', args.dst, attributes={'schema': schema}, force=True)

        yt.run_map(
            Mapper(interesting_message_ids),
            src_list,
            args.dst,
        )
