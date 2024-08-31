import json

import alice.nlu.proto.dataset_info.dataset_info_pb2 as dataset_info_pb

import click
import google.protobuf.json_format as pb_json


def merge_dataset_infos(from_: dataset_info_pb.TIntentDatasets, to_: dataset_info_pb.TIntentDatasets):
    for intent_name, dataset_list in from_.IntentDatasets.items():
        to_.IntentDatasets.get_or_create(intent_name).MergeFrom(dataset_list)


@click.command()
@click.argument('dataset-info-files', nargs=-1, type=click.Path(exists=True, dir_okay=False))
@click.option('--result-file', required=True, type=click.Path())
def main(dataset_info_files, result_file):
    merged = dataset_info_pb.TIntentDatasets()

    for dataset_info_file in dataset_info_files:
        with open(dataset_info_file, 'r', encoding='utf-8') as f:
            pb_info = pb_json.Parse(f.read(), dataset_info_pb.TIntentDatasets())
            merge_dataset_infos(pb_info, merged)

    with open(result_file, 'w', encoding='utf-8') as f:
        merged_as_dict = pb_json.MessageToDict(merged)
        json.dump(merged_as_dict, f, ensure_ascii=False, indent=4, sort_keys=True)
        f.write('\n')
