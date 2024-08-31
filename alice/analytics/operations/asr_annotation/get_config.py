# -*-coding: utf8 -*-
#!/usr/bin/env python

import json
import nirvana.job_context as nv

ctx = nv.context()
parameters = json.loads(ctx.get_parameters().get('kwargs', '{}'))
inputs = ctx.get_inputs()
outputs = ctx.get_outputs()


CONFIG_MAP = {
    "toloka_speakers_ru":  {
        "project_id": "29134",
        "base_pool_id": "8465641",
    },
    "toloka_maps_ru":  {
        "project_id": "29134",
        "base_pool_id": "8465641",
    },
    "toloka_general_ru":  {
        "project_id": "29134",
        "base_pool_id": "8465641",
    },
    "toloka_speakers_ru_elite":  {
        "project_id": "29134",
        "base_pool_id": "14881964",
    },
    "toloka_maps_ru_elite":  {
        "project_id": "29134",
        "base_pool_id": "14881964",
    },
    "toloka_general_ru_elite":  {
        "project_id": "29134",
        "base_pool_id": "14881964",
    },
    "toloka_speakers_ru_self_employed":  {
        "project_id": "37761",
        "base_pool_id": "16388974",
    },
    "toloka_speakers_ru_skip_full_text":  {
        "project_id": "37761",
        "base_pool_id": "15763925",
        "skip_full_text": True
    },
    "toloka_full_text_with_artificial":  {
        "project_id": "41543",
        "base_pool_id": "24252743",
        "hitType": "honey2021_id41543",
        "do_not_use_mds": True,
        "lang": "ru"
    },
    "yang_ru_low_priority":  {
        "project_id": "6304",
        "base_pool_id": "14577601",
        "toloka-token": "robot-yang-voicestr_token"
    },
    "yang_ru_high_priority":  {
        "project_id": "6304",
        "base_pool_id": "14577601",
        "toloka-token": "robot-yang-voiceup_token"
    },
    "yang_ru_tom_annotations":  {
        "project_id": "6304",
        "base_pool_id": "35493285",
        "toloka-token": "robot-yang-voicemod_token"
    },
    "yang_ru_high_quality":  {
        "project_id": "6304",
        "base_pool_id": "23738203",
        "toloka-token": "robot-yang-voiceup_token"
    },
    "yang_ru_regular":  {
        "project_id": "6304",
        "base_pool_id": "14577601",
        "toloka-token": "robot-yang-voicereg_token"
    },
    "yang_ru_moderators":  {
        "project_id": "8402",
        "base_pool_id": "20361916",
        "toloka-token": "robot-yang-voicemod_token"
    },
    "yang_ru_context":  {
        "project_id": "16367",
        "base_pool_id": "39215424",
        "toloka-token": "robot-yang-voicemod_token"
    }
}


def get_config_value(config_name):
    config_value = CONFIG_MAP.get(config_name, {})
    if config_value:
        config_value['projectGroovyPredicate'] = config_value['project_id']
        config_value['poolGroovyPredicate'] = config_value['base_pool_id']
        if "ru" in config_name:
            config_value["lang"] = "ru"
        if "yang" in config_name:
            config_value["encryptedOauthToken"] = config_value["toloka-token"]
        elif 'hitType' not in config_value:
            config_value["hitType"] = "honey2020_id29134"
    return config_value


def main():
    config_name = parameters.get('config_name')
    config_value = get_config_value(config_name)

    with open(outputs.get('output1'), 'w') as f:
        json.dump(config_value, f, indent=4, ensure_ascii=False)

    if inputs.has("input1"):
        with open(inputs.get("input1"), "r") as f:
            dynamic_overlap = f.read()
            min_overlap = str(parameters.get('min_overlap', 0))
            max_overlap = str(parameters.get('max_overlap', 0))
            is_spotter_metrics_only = "true" if parameters.get('is_spotter_metrics_only', False) else "false"
            dynamic_overlap = dynamic_overlap.replace('MIN_OVERLAP', min_overlap).replace('MAX_OVERLAP', max_overlap)
            dynamic_overlap = dynamic_overlap.replace('IS_SPOTTER_METRICS_ONLY', is_spotter_metrics_only)
            dynamic_overlap = {"dynamic_overlap": dynamic_overlap}

        with open(outputs.get('output2'), 'w') as f:
            json.dump(dynamic_overlap, f, indent=4, ensure_ascii=False)


if __name__ == '__main__':
    main()
