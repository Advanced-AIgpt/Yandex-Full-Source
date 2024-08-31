# -*-coding: utf8 -*-
#!/usr/bin/env python

import json
import sys
import nirvana.job_context as nv

reload(sys)
sys.setdefaultencoding('utf-8')

ctx = nv.context()
parameters = json.loads(ctx.get_parameters().get('kwargs', '{}'))
inputs = ctx.get_inputs()
outputs = ctx.get_outputs()


def move_json(records):
    result = []
    for record in records:
        input_values = {}
        input_values["audio"] = record["url"]
        input_values["mds_key"] = record["mds_key"]
        raw_assesments = record.get('raw_assesments') or []
        for raw_assesment in raw_assesments:
            res = {}
            res["outputValues"] = raw_assesment
            res["inputValues"] = input_values
            res["workerId"] = raw_assesment["workerId"]
            res["submitTs"] = raw_assesment["submitTs"]
            res["taskId"] = None
            result.append(res)
    return result


def main():
    with open(inputs.get("input1"), "r") as f:
       records = json.load(f)

    moved_json = move_json(records)

    with open(outputs.get('output1'), 'w') as f:
        json.dump(moved_json, f, indent=4, ensure_ascii=False)


if __name__ == '__main__':
    main()
