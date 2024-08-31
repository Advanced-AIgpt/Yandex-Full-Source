# -*- coding: utf-8 -*-

from __future__ import print_function
from __future__ import absolute_import
from __future__ import unicode_literals
from yql.api.v1.client import YqlClient

import argparse
import json
import logging


def run(args):
    logging.basicConfig(format='%(levelname)-8s %(asctime)-27s %(message)s', level=logging.INFO)

    logging.info('Started aggregating tables')

    with open(args.scenario_list, 'r') as file:
        scenario_list = json.load(file)["scenarios"]

    if args.request_data_folder[-1] == '/':
        args.request_data_folder = args.request_data_folder[:-1]
    request_data_template = args.request_data_folder + "/{}/" + args.request_data_table_name

    if (len(scenario_list) == 0):
        logging.warning('Winner scenario list is empty, so table with result of aggregation will be empty!')

    query_header = '''
        PRAGMA yt.InferSchema = '100';

        $winner_scenarios = "{0}";
        $request_ids = "{1}";
        $result = "{2}";

        $numerated_request_ids = SELECT (ROW_NUMBER() OVER w) - 1 AS row_id, text, reqid
        FROM $request_ids
        WINDOW w AS (ORDER BY text, reqid);

        $winners_with_reqid = SELECT winner_scenario, discarded_scenarios, reqid
        FROM $winner_scenarios as t1 JOIN $numerated_request_ids as t2 ON t1.id == cast(t2.row_id as string);
    '''.format(args.request_winner_scenarios, args.request_ids, args.classification_results)

    query = [query_header]
    truncation = "WITH TRUNCATE"
    for scenario in scenario_list:
        join_query = '''
            $request_data_on_{0} = "{1}";
            INSERT INTO $result {2}
            SELECT CAST(t2.action as String?) as action, winner_scenario, discarded_scenarios, t2.* WITHOUT t2.action
            FROM (SELECT * FROM $winners_with_reqid WHERE winner_scenario == "{0}") as t1 JOIN $request_data_on_{0} as t2 ON t1.reqid == t2.req_id;
        '''.format(scenario, request_data_template.format(scenario if scenario != "other" else "vins"), truncation)
        query.append(join_query)
        truncation = ""

    with YqlClient(db=args.cluster, token=args.yql_token) as client:
        request = client.query('\n'.join(query), syntax_version=1)
        request.run()

    logging.info('YQL share link: {}'.format(request.share_url))

    request.get_results()
    if (not request.is_success):
        logging.error('Incorrect YQL query!')
        logging.error('Request is %s.' % request.status)
        if request.errors:
            logging.error('Returned errors:')
            for error in request.errors:
                logging.error(' - ' + str(error))
        raise "Incorrect YQL query"

    logging.info('Finished aggregating tables.')


def main():
    argument_parser = argparse.ArgumentParser()

    argument_parser.add_argument(
        '--scenario_list',
        required=True,
        help='JSON with list of scenarios',
    )
    argument_parser.add_argument(
        '--request_winner_scenarios',
        required=True,
        help='MR table with scenarios, chosen on request classification',
    )
    argument_parser.add_argument(
        '--request_ids',
        required=True,
        help='MR table with request ids for each winner scenario (rows in the same order)',
    )
    argument_parser.add_argument(
        '--cluster',
        required=True,
        help='YT cluster',
    )
    argument_parser.add_argument(
        '--request_data_folder',
        required=True,
        help='Path to folder with request data for each scenario',
    )
    argument_parser.add_argument(
        '--request_data_table_name',
        required=True,
        help='Name of table with request data for each scenario',
    )
    argument_parser.add_argument(
        '--yql_token',
        required=True,
        help='YQL token',
    )
    argument_parser.add_argument(
        '--classification_results',
        required=True,
        help='MR table with result of aggregation',
    )

    run(argument_parser.parse_args())
