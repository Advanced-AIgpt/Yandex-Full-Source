import os
import json
import yatest.common

from alice.analytics.ww.arabic.asr_annotation.aggregator.lib.aggregate import process_item


def test_aggregation():
    input_data_path = 'test_data.in.json'
    with open(yatest.common.test_source_path(input_data_path)) as f:
        data = json.load(f)

    for item in data:
        aggregation_results = process_item(item['answers_raw'])
        item.update(aggregation_results)

    output_filename = os.path.basename(input_data_path).replace('in', 'out')
    output_path = yatest.common.output_path(output_filename)

    with open(output_path, 'w') as f:
        json.dump(data, f, indent=4, sort_keys=True, ensure_ascii=False)

    return yatest.common.canonical_file(output_path, local=True)
