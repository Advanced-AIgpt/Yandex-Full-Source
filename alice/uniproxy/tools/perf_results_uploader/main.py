import alice.uniproxy.library.perf_tester.events as events
import argparse
import json
import sys
import voicetech.common.lib.utils as utils
import yt.wrapper as yt


logger = utils.initialize_logging(__name__)


def to_bool(value):
    if isinstance(value, bool):
        return value
    if value.lower() == 'true':
        return True
    if value.lower() == 'false':
        return False
    raise argparse.ArgumentTypeError('Unexpected boolean value: {}'.format(value))


def go(args):
    entries = json.load(sys.stdin)

    es = events.ALL_EVENTS

    timings = []
    for entry in entries:
        directive = entry.get('directive', {})
        header = directive.get('header', {})
        namespace = header.get('namespace', '')
        name = header.get('name', '')
        if namespace == 'Vins' and name == 'UniproxyVinsTimings':
            payload = directive['payload'].copy()
            payload['epoch'] = int(payload['epoch'])
            timings.append({e.NAME: payload.get(e.NAME) for e in es})

    schema = []
    for e in es:
        row = {
            'name': e.NAME,
            'type': e.YT_TYPE
        }
        schema.append(row)

    table_path = yt.TablePath(args.yt_table, append=args.append)

    if not yt.exists(path=table_path):
        yt.create(type='table',
                  path=table_path,
                  attributes={'schema': schema},
                  force=True)
    try:
        yt.write_table(table=table_path,
                       input_stream=timings,
                       format=yt.JsonFormat())
    except Exception:
        yt.erase(path=table_path)
        raise


def main():
    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument(
        '--yt-table',
        type=str,
        required=True,
        help='Path to the output YT table')
    parser.add_argument(
        '--append',
        type=to_bool,
        nargs='?',
        const=True,
        default=False,
        help='When true, data will be appended to the existing table')
    go(args=parser.parse_args())
