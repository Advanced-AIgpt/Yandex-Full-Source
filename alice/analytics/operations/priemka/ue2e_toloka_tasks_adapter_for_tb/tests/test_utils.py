import os
import json
import yatest.common
from alice.analytics.operations.priemka.ue2e_toloka_tasks_adapter_for_tb.utils import convert_toloka_task_to_tb_format


def test_convert_toloka_task_to_tb_format():
    input_data_path = 'test_data.in.json'
    with open(yatest.common.test_source_path(input_data_path)) as f:
        data = json.load(f)

    result = list(map(convert_toloka_task_to_tb_format, data))

    output_filename = os.path.basename(input_data_path).replace('in', 'out')
    output_path = yatest.common.output_path(output_filename)

    with open(output_path, 'w') as f:
        json.dump(result, f, indent=4, sort_keys=True, ensure_ascii=False)

    return yatest.common.canonical_file(output_path, local=True)


def test_convert_toloka_task_to_tb_format_w_options():
    input_data_path = 'test_data.in.json'
    with open(yatest.common.test_source_path(input_data_path)) as f:
        data = json.load(f)

    result = [
        convert_toloka_task_to_tb_format(data[0], need_append_app=True),
        convert_toloka_task_to_tb_format(data[2], need_append_app=True),
        convert_toloka_task_to_tb_format(data[3], need_append_app=True, override_app="tv"),
        convert_toloka_task_to_tb_format(data[4], need_append_app=True, override_app="tv"),
    ]

    output_filename = os.path.basename(input_data_path).replace('in', 'out_w_opts')
    output_path = yatest.common.output_path(output_filename)

    with open(output_path, 'w') as f:
        json.dump(result, f, indent=4, sort_keys=True, ensure_ascii=False)

    return yatest.common.canonical_file(output_path, local=True)
