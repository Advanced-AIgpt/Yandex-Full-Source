from collections import defaultdict
import json
from typing import Dict, List

import alice.nlu.proto.dataset_info.dataset_info_pb2 as dataset_info_pb

import click
import google.protobuf.json_format as pb_json


def get_intent_guidelines(datasets_info: dataset_info_pb.TIntentDatasets) -> Dict[str, List[str]]:
    intent2guidelines = defaultdict(set)

    for intent_name, intent_info_list in datasets_info.IntentDatasets.items():
        for intent_info in intent_info_list.DatasetInfos:
            if not intent_info.Deprecated and intent_info.HasField('Guideline'):
                intent2guidelines[intent_name].add(intent_info.Guideline)

    return {intent: sorted(guidelines) for intent, guidelines in intent2guidelines.items()}


@click.command()
@click.option('--datasets-info-file', '-i', required=True, type=click.Path(exists=True, dir_okay=False))
@click.option('--output-json', '-o', required=True, type=click.Path(exists=False))
def main(datasets_info_file, output_json):
    with open(datasets_info_file, 'r', encoding='utf-8') as f:
        datasets_info = pb_json.Parse(f.read(), dataset_info_pb.TIntentDatasets())

    intent2guidelines = get_intent_guidelines(datasets_info)

    with open(output_json, 'w', encoding='utf-8') as f:
        json.dump(intent2guidelines, f, ensure_ascii=False, sort_keys=True, indent=4)
