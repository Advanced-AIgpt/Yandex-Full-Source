# coding: utf-8
from __future__ import unicode_literals

import argparse
import yt.wrapper as yt
import re
import pandas as pd
import logging
import logging.config
import math
import numpy as np
import json

from collections import defaultdict
from operator import itemgetter

from vins_core.utils.config import get_setting
from vins_core.utils.strings import smart_unicode


logger = logging.getLogger(__name__)


def set_logging(level='INFO'):
    logging.config.dictConfig({
        'version': 1,
        'disable_existing_loggers': False,
        'formatters': {
            'standard': {
                'format': '[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s',
            },
        },
        'handlers': {
            'console': {
                'class': 'logging.StreamHandler',
                'formatter': 'standard',
            },
        },
        'loggers': {
            '': {
                'handlers': ['console'],
                'level': level,
                'propagate': True,
            },
        },
    })


_yt_client = yt.YtClient(
    proxy=get_setting('YT_PROXY', default='hahn', prefix=''),
    token=get_setting('YT_TOKEN', default='', prefix='')
)


def _parse_column_name(name):
    m = re.match(r'(?P<name>[^\[]*)(?:\[(?P<index>[^\]]*)\])?', name).groupdict()
    return m['name'], int(m['index']) if m['index'] is not None else None


def _extract_string(row, col, index=None):
    string = None
    if col:
        try:
            string = row[col]
            if index is not None:
                string = string[index]
            string = smart_unicode(string)
        except IndexError:
            raise ValueError('Unable to retrieve data at index %d for row=%r', index, row)
    return string


_renames = {
    'personal_assistant.scenarios.search.nav_url': 'personal_assistant.scenarios.search',
    'personal_assistant.scenarios.search.translate_stub': 'personal_assistant.scenarios.search',
    'personal_assistant.scenarios.search.calculator': 'personal_assistant.scenarios.search',
    'personal_assistant.scenarios.search.object': 'personal_assistant.scenarios.search',
    'personal_assistant.scenarios.search.factoid': 'personal_assistant.scenarios.search',
    'personal_assistant.scenarios.find_poi.scroll.by_index':
        'personal_assistant.scenarios.find_poi__scroll__by_index',
    'personal_assistant.scenarios.search.serp.factoid_src': 'personal_assistant.scenarios.search__factoid_src',
    'personal_assistant.scenarios.search.factoid.serp': 'personal_assistant.scenarios.search',
    'personal_assistant.scenarios.search.serp.serp': 'personal_assistant.scenarios.search__serp',
    'personal_assistant.scenarios.search.serp.factoid_call': 'personal_assistant.scenarios.search__factoid_call',
    'personal_assistant.scenarios.find_poi.scroll.next': 'personal_assistant.scenarios.find_poi__scroll__next',
    'personal_assistant.scenarios.find_poi.scroll.prev': 'personal_assistant.scenarios.find_poi__scroll__prev',
    'personal_assistant.scenarios.search.object.factoid_src': 'personal_assistant.scenarios.search__factoid_src',
    'personal_assistant.scenarios.search.factoid.factoid_src': 'personal_assistant.scenarios.search__factoid_src',
    'personal_assistant.scenarios.search.nav_url.serp': 'personal_assistant.scenarios.search',
    'personal_assistant.scenarios.search.object.serp': 'personal_assistant.scenarios.search',
    'personal_assistant.scenarios.search.entity_name': 'personal_assistant.scenarios.search',
    # some new intents from navi
    'personal_assistant.scenarios.search.map_search_url.serp': 'personal_assistant.scenarios.search__serp',
    'personal_assistant.scenarios.search.map_search_url.show_on_map':
        'personal_assistant.scenarios.search__show_on_map',
    'personal_assistant.scenarios.search.entity_name.serp': 'personal_assistant.scenarios.search__serp',
    'personal_assistant.scenarios.search.entity_name.show_on_map': 'personal_assistant.scenarios.search__show_on_map',
    'personal_assistant.scenarios.search.factoid.show_on_map': 'personal_assistant.scenarios.search__show_on_map',
    'personal_assistant.scenarios.search.entity_name.factoid_src': 'personal_assistant.scenarios.search__factoid_src',
    'personal_assistant.scenarios.search.map_search_url.factoid_src':
        'personal_assistant.scenarios.search__factoid_src',
    'personal_assistant.scenarios.search.calculator.factoid_src': 'personal_assistant.scenarios.search__factoid_src',
    'personal_assistant.scenarios.search.translate_stub.serp': 'personal_assistant.scenarios.search__serp',
    'personal_assistant.scenarios.search.calculator.serp': 'personal_assistant.scenarios.search__serp',
    'personal_assistant.scenarios.search.map_search_url.factoid_call':
        'personal_assistant.scenarios.search__factoid_call',
    'personal_assistant.scenarios.search.entity_name.factoid_call':
        'personal_assistant.scenarios.search__factoid_call',
    'personal_assistant.scenarios.market.market.ellipsis': 'personal_assistant.scenarios.market__market__ellipsis'
}


_intents_with_many_dots = 'personal_assistant.(handcrafted.(autoapp|drive|quasar)|scenarios.(common|hny|quasar))'


def vins_intent_renames_fix(intent_name):
    default_intent = 'personal_assistant.internal.session_start'
    if not intent_name:
        return default_intent
    if intent_name in _renames:
        return _renames[intent_name]
    elif intent_name.count('.') == 2:
        return intent_name
    elif intent_name.count('.') >= 3 and re.match(_intents_with_many_dots, intent_name):
        parts = intent_name.split('.')
        result = '.'.join(parts[:4])
        if len(parts) > 4:
            result = result + '__' + '__'.join(parts[4:])
        return result
    elif intent_name.count('.') == 3:
        app_name, prefix, parent_intent, suffix = intent_name.split('.')
        return app_name + '.' + prefix + '.' + parent_intent + '__' + suffix
    else:
        logger.error('Unknown intent name %s; setting default intent %s', intent_name, default_intent)
        return default_intent


def _get_tables_list(input_tables, last, start_date, end_date):
    sorted_tables = sorted(_yt_client.list(input_tables))
    if last is not None:
        tables = sorted_tables[-last:]
    elif start_date is not None and end_date is not None:
        tables = filter(lambda date: start_date <= date <= end_date, sorted_tables)
    elif start_date is not None:
        tables = filter(lambda date: date >= start_date, sorted_tables)
    else:
        tables = sorted_tables
    return tables


def _rename_intent(intent, renames):
    renaming_found = False
    for pattern, new_intent in renames['true_intent'].iteritems():
        if re.match(pattern, intent):
            intent = new_intent
            renaming_found = True
            break
    if not renaming_found:
        raise ValueError('Renaming not found for intent "%s", please fix file with renamings' % intent)
    return intent


def _collect_data(
    input_tables, last, text_col, intent_col, prev_intent_col, start_date, end_date, app, app_col, renames,
    device_state_col, input_table
):
    text_col, text_col_index = _parse_column_name(text_col)
    output = []
    if input_table:
        tables = [input_table]
    else:
        tables = _get_tables_list(input_tables, last, start_date, end_date)
    for table in tables:
        table_path = yt.ypath_join(input_tables, table)
        logger.info('Start processing table %s', table_path)
        for row in _yt_client.read_table(table_path):
            if app_col:
                curr_app = _extract_string(row, app_col)
                if app and curr_app not in app:
                    continue
            intent = _extract_string(row, intent_col)

            if intent == 'unknown':
                continue

            if renames:
                intent = _rename_intent(intent, renames)

            text = _extract_string(row, text_col, text_col_index)

            output_row = {'text': text, 'intent': intent}
            if prev_intent_col:
                prev_intent = _extract_string(row, prev_intent_col)
                # Fix yt-specific intent names
                prev_intent = vins_intent_renames_fix(prev_intent)

                if prev_intent and prev_intent.startswith('personal_assistant.scenarios.external_skill'):
                    # skip entrance or inner states of external skills
                    continue
                output_row['prev_intent'] = prev_intent

            if device_state_col:
                device_state_string = _extract_string(row, device_state_col).encode('utf-8')
                device_state = json.dumps(json.loads(device_state_string))
                output_row['device_state'] = device_state
            output.append(output_row)
    return output


def _get_intents_and_probs(intent_to_items, sampling='freq'):
    if sampling == 'freq':
        def sampling_method(x):
            return x
    elif sampling == 'sqrt_freq':
        def sampling_method(x):
            return math.sqrt(x)
    elif sampling == 'uniform':
        def sampling_method(x):
            return 1
    else:
        raise ValueError('Unable to define sampling method "{}"'.format(sampling))
    weights = {intent: sampling_method(len(items)) for intent, items in intent_to_items.iteritems()}
    sum_weights = float(sum(weights.itervalues()))
    return {intent: weight / sum_weights for intent, weight in weights.iteritems()}


def _hashable_row_repr(row):
    return tuple(sorted(row.iteritems(), key=itemgetter(0)))


def _sample_dataset(data, size, min_intent_size, sampling='freq'):
    intent_to_items = defaultdict(list)
    for row in data:
        intent_to_items[row['intent']].append(_hashable_row_repr(row))

    weights = _get_intents_and_probs(intent_to_items, sampling=sampling)
    output = []
    for intent, intent_data in intent_to_items.iteritems():
        unique_intent_data = list(set(intent_data))
        requested_size = int(weights[intent] * size)
        if requested_size < min_intent_size:
            logger.warning(
                'Requested size for intent "%s" (%d samples) is less than min_intent_size=%d',
                intent, requested_size, min_intent_size
            )
            requested_size = min_intent_size
        if requested_size > len(unique_intent_data):
            logger.warning(
                "Can't sample %d samples for intent %s: only %d found. Try to increase dataset size",
                requested_size, intent, len(unique_intent_data)
            )
            requested_size = len(unique_intent_data)

        requested_data_idx = np.random.choice(range(len(unique_intent_data)), size=requested_size, replace=False)
        for idx in requested_data_idx:
            output.append(dict(unique_intent_data[idx]))
    return output


def main():
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument(
        '-i', '--input-tables', dest='input_tables', help='Input YT tables',
        default='//home/voice/dialog/intents/daily')
    parser.add_argument('--input-table', dest='input_table',
                        help='Input table (if specified, overwrites "--input-tables" target)')
    parser.add_argument('--last', help='process only last N tables', dest='last', type=int, default=None)
    parser.add_argument('--start-date', dest='start_date', help='start date')
    parser.add_argument('--end-date', dest='end_date', help='end date')
    parser.add_argument(
        '--sampling', dest='sampling', choices=['freq', 'sqrt_freq', 'uniform'], default='freq', help='sampling method')
    parser.add_argument('--dataset-size', help='Requested dataset size', type=int, default=50000)
    parser.add_argument(
        '--app', dest='app', help='select target app(s) separated by space (all apps by default)', nargs='+')
    parser.add_argument('-o', '--output-file', dest='output_file', required=True, help='Output TSV file')
    parser.add_argument('--text-col', dest='text_col', help='Text column', default='dialog[-1]')
    parser.add_argument('--intent-col', dest='intent_col', help='Intent column', default='toloka_intent')
    parser.add_argument('--app-col', dest='app_col', help='App column')
    parser.add_argument('--prev-intent-col', dest='prev_intent_col', help='Prev intent column')
    parser.add_argument('--device-state-col', dest='device_state_col', help='Device state column')
    parser.add_argument(
        '--min-intent-size', dest='min_intent_size', help='Minimum number of samples per intent for stratified version',
        type=int, default=100)
    parser.add_argument('--renames', dest='renames', help='path to file with renamings')
    parser.add_argument('--no-sample', dest='no_sample', help='whether to leave dataset as is', action='store_true')

    args = parser.parse_args()

    output = _collect_data(
        input_tables=args.input_tables,
        last=args.last,
        start_date=args.start_date,
        end_date=args.end_date,
        app=args.app,
        text_col=args.text_col,
        intent_col=args.intent_col,
        app_col=args.app_col,
        prev_intent_col=args.prev_intent_col,
        renames=json.load(open(args.renames)) if args.renames else {},
        device_state_col=args.device_state_col,
        input_table=args.input_table
    )
    if not args.no_sample:
        output = _sample_dataset(output, args.dataset_size, args.min_intent_size)

    output = pd.DataFrame.from_records(output)
    if not args.no_sample:
        output.drop_duplicates(inplace=True)
    output.sort_values(output.columns.tolist(), inplace=True)
    output.to_csv(
        path_or_buf=args.output_file, sep=b'\t', index=False, encoding='utf-8'
    )
    logger.info(
        '%d %s items were successfully dumped to %s',
        len(output), 'non-unique' if args.no_sample else 'unique', args.output_file
    )


def do_main():
    set_logging()
    main()


if __name__ == "__main__":
    do_main()
