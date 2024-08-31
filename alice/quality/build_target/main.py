# -*- coding: utf-8 -*-

from library.python import resource
import yt.wrapper as yt
import intent_renamer

import os
import argparse

from biases import get_column, mark_column_name, target_biases


class BuildTarget(object):
    def __init__(self, args):
        self._classification_bias = args.classification_bias
        self._debug_target = args.debug_target
        self._important_parts = args.important_parts
        self._ignore_gc = args.ignore_gc
        self._raw_scores = args.raw_scores
        self._mapping = map(lambda x: os.path.basename(x), args.mapping) if args.mapping else None
        self._scenarios_list = args.scenarios_list.split(',')
        self._client = args.client
        self._flags = set(args.additional_flags.split(',')) if args.additional_flags else set()
        if not args.no_search_bias:
            self._flags.add('search_bias_toloka')
        if self._classification_bias:
            self._flags.add('classification_bias')
        if not self._raw_scores:
            self._flags.add('not_raw_scores')
        if args.add_gc:
            self._flags.add('add_gc')
        if not args.disable_vins_bias:
            self._flags.add('vins_fallback')
        if args.search_from_vins:
            self._flags.add('search_from_vins')
        if args.search_from_toloka:
            self._flags.add('search_from_toloka')

    def start(self):
        self._renamer = intent_renamer.IntentRenamer(self._mapping if self._mapping else (
            resource.find('/toloka_intent_renames_for_vins'),
        ))

    def _normalize_parts(self, mark, has_good):
        if mark == 0.5:
            mark = 0 if (has_good and not self._important_parts) or self._raw_scores else 1
        return int(mark)

    def __call__(self, row):
        raw_toloka_intent = row['toloka_intent']
        toloka_intent = self._renamer(row['toloka_intent'], intent_renamer.IntentRenamer.By.TRUE_INTENT)

        for scenario in self._scenarios_list:
            if get_column(row, mark_column_name(scenario), None) is None and scenario != 'gc':
                return

        marks = {scenario : get_column(row, mark_column_name(scenario), 0) for scenario in self._scenarios_list}
        if self._ignore_gc and 'gc' in self._scenarios_list:
            marks['gc'] = 0

        vins_intent = get_column(row, 'base_vins_intent', '')
        new_vins_intent = get_column(row, 'vins_intent', '')
        # vins_intent = new_vins_intent

        has_good = max(marks.values()) == 1
        targets = {scenario : self._normalize_parts(marks[scenario], has_good) for scenario in marks}

        applied_biases = []

        for bias in target_biases:
            if bias.is_enabled(self._client, self._scenarios_list, self._flags):
                targets, applied, name = bias.call(targets, marks, toloka_intent, raw_toloka_intent, vins_intent, new_vins_intent, row)
                if applied:
                    applied_biases.append(name)

        features = get_column(row, 'features', None)
        req_id = get_column(row, 'reqid', None)
        forced_confident = get_column(row, 'forced_confident', None)
        answer = {
            'text': row['text'],
            'features': features,
            'reqid': req_id,
            'forced_confident': forced_confident
        }
        for scenario in marks:
            answer[scenario] = int(targets[scenario])

        if self._debug_target:
            for scenario in marks:
                answer[mark_column_name(scenario)] = marks[scenario]
            answer['vins_intent'] = vins_intent
            answer['new_vins_intent'] = new_vins_intent
            answer['toloka_vins_intent'] = toloka_intent
            answer['raw_toloka_intent'] = raw_toloka_intent
            answer['applied_biases'] = ','.join(applied_biases)

        yield answer


def run(args):
    yt.config['proxy']['url'] = args.proxy
    yt.run_map(
        BuildTarget(args),
        source_table=args.input_table,
        destination_table=args.output_table,
        local_files=args.mapping,
    )


def main():
    argument_parser = argparse.ArgumentParser()

    argument_parser.add_argument(
        '-p', '--proxy',
        required=True,
        help='YT proxy',
    )
    argument_parser.add_argument(
        '-i', '--input-table',
        required=True,
        help='input table path',
    )
    argument_parser.add_argument(
        '-o', '--output-table',
        required=True,
        help='output table path',
    )
    argument_parser.add_argument(
        '-c', '--classification-bias',
        action='store_true',
        help='try boost target with classification markup',
    )
    argument_parser.add_argument(
        '--no-search-bias',
        action='store_true',
        help='bias for search is ignored',
    )
    argument_parser.add_argument(
        '-d', '--debug-target',
        action='store_true',
        help='add columns with ue2e marks to result table',
    )
    argument_parser.add_argument(
        '--search-from-vins',
        action='store_true',
        help='transform vins search into mm search',
    )
    argument_parser.add_argument(
        '--search-from-toloka',
        action='store_true',
        help='transform toloka search into mm search',
    )
    argument_parser.add_argument(
        '--important-parts',
        action='store_true',
        help='make score for parts same as for goods',
    )
    argument_parser.add_argument(
        '--ignore-gc',
        action='store_true',
        help='gc score becomes 0 if any other scenario has 1',
    )
    argument_parser.add_argument(
        '--raw-scores',
        action='store_true',
        help='Build target based on toloka goods. Only marks will only decrease',
    )
    argument_parser.add_argument(
        '--disable-vins-bias',
        action='store_true',
        help='Do not give vins 1 mark if all marks are zeros',
    )
    argument_parser.add_argument(
        '--add-gc',
        action='store_true',
        help='Add gc target from vins',
    )
    argument_parser.add_argument(
        '-m', '--mapping',
        action='append',
        help='intent mapping files',
    )
    argument_parser.add_argument(
        '--scenarios_list',
        required=True,
        help='List of scenarios',
    )
    argument_parser.add_argument(
        '--client',
        required=True,
        help='Client type',
    )
    argument_parser.add_argument(
        '--additional_flags',
        required=False,
        help='Additional exp flags',
    )

    run(argument_parser.parse_args())
