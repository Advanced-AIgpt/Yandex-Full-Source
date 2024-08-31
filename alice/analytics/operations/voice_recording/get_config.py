# -*-coding: utf8 -*-
import json
import random
import nirvana.job_context as nv

ctx = nv.context()
parameters = json.loads(ctx.get_parameters().get('kwargs', '{}'))
inputs = ctx.get_inputs()
outputs = ctx.get_outputs()

RECORDING_CONFIG = {
    "ru": {
        "mobile": {
            "singleProjectId": '39074',
            "singlePoolId": '24247342',
        },
        "web": {
            "singleProjectId": '22047',
            "singlePoolId": '5308213',
        }
    }
}


ACCEPTANCE_CONFIG = {
    "ru": {
        "singleProjectId": '11435',
        "singlePoolId": '27994377',
        "threshold": 0.79,
        "align_treshhold": 0.8
    }
}


def get_pool_options(abc_id):
    pool_options = {
        "metadata": {
            "abc_service_id": [abc_id]
        }
    }
    return pool_options


def get_toloka_options(lang, device, base_config):
    toloka_options = base_config[lang].get(device) or base_config[lang]
    return toloka_options


def main():
    if parameters.get('stage') == 'recording':
        base_config = RECORDING_CONFIG
        device = parameters.get('device')
    elif parameters.get('stage') == 'acceptance':
        base_config = ACCEPTANCE_CONFIG
        device = None
    else:
        base_config = {}
        device = None
    pool_options = get_pool_options(parameters.get('abc_id'))
    toloka_options = get_toloka_options(parameters.get('lang'), device, base_config)

    if inputs.has("input1"):
        tasks = []
        with open(inputs.get("input1"), "r") as f:
            for line in f:
                s = line.strip().split('\t')
                num = int(s[1]) if len(s) == 2 else 1
                for i in range(num):
                    tasks.append({'inputValues': {'query': s[0]}})
        random.shuffle(tasks)

        with open(outputs.get('output1'), 'w') as f:
            json.dump(tasks, f, indent=4, ensure_ascii=False)

    with open(outputs.get('output2'), 'w') as f:
        json.dump(pool_options, f, indent=4, ensure_ascii=False)

    with open(outputs.get('output3'), 'w') as f:
        json.dump(toloka_options, f, indent=4, ensure_ascii=False)


if __name__ == '__main__':
    main()
