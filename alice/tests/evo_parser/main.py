import argparse
import json
import logging
import os
from google.protobuf.json_format import MessageToDict

from alice.tests.evo_parser.lib.evo_parser_data_pb2 import TEvoParserRequest
from alice.tests.evo_parser.lib import EvoFailsParser


logger = logging.getLogger(__name__)


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('-t', '--task-id', help='Sandbox task id of ALICE_EVO_INTEGRATION_TESTS_WRAPPER', required=True, type=int)
    parser.add_argument('-r', '--only-release', action='store_true', help='Show only release fails')
    return parser.parse_args()


def main():
    FORMAT = '%(asctime)-15s %(message)s'
    logging.basicConfig(format=FORMAT, level=logging.DEBUG)

    args = parse_args()
    parser = EvoFailsParser(TEvoParserRequest(
        ArcanumToken=os.environ['ARCANUM_TOKEN'],
        SandboxTaskId=args.task_id,
        OnlyRelease=args.only_release,
    ))
    response = parser.parse()
    print(json.dumps(MessageToDict(response), indent=4))
