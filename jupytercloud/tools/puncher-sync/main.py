import argparse
import datetime
import itertools
import pprint
import sys
import urllib.parse

import requests
import library.python.oauth as lpo

from ruamel.yaml import YAML
from ruamel.yaml.comments import CommentedSeq, CommentedMap
from jupytercloud.tools.lib import utils

DEFAULT_PUNCHER_API = 'https://api.puncher.yandex-team.ru/api/dynfw/'


def parse_args():
    parser = argparse.ArgumentParser()

    parser.add_argument(
        '--verbose', '-v',
        action='count',
        help='print to stderr some additional info',
        default=0,
    )
    parser.add_argument(
        '--puncher-api',
        default='https://api.puncher.yandex-team.ru/api/dynfw/',
    )

    subparsers = parser.add_subparsers(required=True)

    dump = subparsers.add_parser('dump')
    dump.set_defaults(handler=dump_macross)

    dst = dump.add_mutually_exclusive_group(required=True)
    dst.add_argument('--src', '--source')
    dst.add_argument('--dst', '--destination')

    dump.add_argument('--dst-file', type=argparse.FileType('w'), default=sys.stdout)

    return parser.parse_args()


class PuncherClient:

    def __init__(self, token, base):
        self.token = token
        self.base = base

    @property
    def headers(self):
        return {
            'Authorization': f'OAuth {self.token}',
            'User-Agent': __name__,
        }

    def request(self, method, uri, query=None, **kwargs):
        url = self.base.removesuffix('/') + '/' + uri.removeprefix('/')

        if query:
            query_str = urllib.parse.urlencode(query, doseq=True)
            url += '?' + query_str

        return self.raw_request(method=method, url=url, **kwargs)

    def raw_request(self, method, url, **kwargs):
        response = requests.request(
            method=method,
            url=url,
            headers=self.headers,
            **kwargs
        )

        response.raise_for_status()

        return response.json()

    def request_iter_pages(self, uri, query=None, **kwargs):
        data = self.request(
            method='GET',
            uri=uri,
            query=query,
            **kwargs
        )

        yield data

        next = data.get('links', {}).get('next')
        while next:
            # NB: in case of port port forwarding and in application for production puncher-api
            next = next.replace(DEFAULT_PUNCHER_API, self.base)

            data = self.raw_request('GET', next, **kwargs)

            yield data

            next = data.get('links', {}).get('next')

    def request_rules(
        self,
        *,
        source=None,
        destination=None,
    ):
        raw_query = {
            'source': source,
            'destination': destination,
            'system': 'puncher',
            'rules': 'exclude_rejected',
        }

        query = {k: v for k, v in raw_query.items() if v is not None}

        pages = self.request_iter_pages('/rules', query=query)

        rules_pages = (page['rules'] for page in pages)

        return itertools.chain(*rules_pages)


def dump_macross(client, args):
    raw_rules = client.request_rules(
        source=args.src,
        destination=args.dst
    )

    def filter_destinations(raw_destinations, destination):
        for raw_destination in raw_destinations:
            if not raw_destination.get('type') == 'macro':
                continue

            if raw_destination.get('machine_name') == destination:
                return False

        return True

    def dump_destinations(raw_destinations):
        return [raw['machine_name'] for raw in raw_destinations]

    rules = CommentedSeq()  # is ascendant of list

    for raw_rule in raw_rules:
        if args.dst and filter_destinations(raw_rule['destinations'], args.dst):
            continue

        if args.src and filter_destinations(raw_rule['sources'], args.src):
            continue

        if raw_rule['until']:
            rule_repr = pprint.pformat(raw_rule)
            logger.warning('rule have non-emptpy "until" field, skipping; \n%s', rule_repr)
            continue

        if raw_rule['system'] == 'cauth':
            continue

        if (
            raw_rule['readonly'] or
            raw_rule['system'] not in ('static', 'puncher') or
            raw_rule['status'] != 'active'
        ):
            rule_repr = pprint.pformat(raw_rule)
            logger.error('rule with strange field, exiting; \n%s', rule_repr)
            sys.exit(1)

        rule = CommentedMap({
            'protocol': raw_rule['protocol'],
            'ports': raw_rule['ports'],
            'comment': raw_rule['comment'],
            'id': raw_rule['id']
        })

        if args.src:
            rule['destinations'] = dump_destinations(raw_rule['destinations'])

        if args.dst:
            rule['sources'] = dump_destinations(raw_rule['sources'])

        if raw_rule.get('locations'):
            rule['locations'] = raw_rule['locations']

        rule.yaml_set_start_comment(f"updated at {raw_rule['updated']}")

        rules.append(rule)

    logger.info('fetched %d rules', len(rules))

    dst_name = f"destination {args.dst}" if args.dst else f"source {args.src}"

    rules.yaml_set_start_comment(
        f"Generated by {__file__} for {dst_name} at {datetime.datetime.now()}"
    )

    yaml = YAML()
    yaml.dump(rules, args.dst_file)


def get_oauth_token():
    client_id = "a4b9cc023f7244e8b2c7b4fa47c444a6"
    client_secret = "7c3f2d5c972e43d6aa5d3479e29e3d8e"

    return lpo.get_token(
        client_id=client_id,
        client_secret=client_secret,
    )


def main():
    args = parse_args()

    global logger
    logger = utils.setup_logging(__name__, args.verbose)

    token = get_oauth_token()

    client = PuncherClient(token, base=args.puncher_api)

    args.handler(client, args)
