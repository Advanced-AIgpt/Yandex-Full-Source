# -*-coding: utf8 -*-
import json
import nirvana.job_context as nv

ctx = nv.context()
parameters = json.loads(ctx.get_parameters().get('kwargs', '{}'))
inputs = ctx.get_inputs()
outputs = ctx.get_outputs()

COLUMNS = ['text', 'app', 'mds_key', 'ticket']

def check_record(record):
    return set(COLUMNS).issubset(set(record.keys()))


def update_record(record):
    record['mds_key'] = "/".join(record['mds_key'].split("/")[-2:])
    record['voice_url'] = "https://speechbase.voicetech.yandex-team.ru/getfile/" + record['mds_key']
    record['text'] = u" ".join(record['text'].decode('utf-8').lower().replace(u"ё", u"е").split()).encode('utf-8')
    record['lang'] = parameters.get('lang')
    return record


def main():
    records = []
    with open(inputs.get("input1"), "r") as f:
        columns = COLUMNS
        for line in f:
            rec = {}
            s = [i.strip('" ') for i in line.strip().split('\t')]
            if set(COLUMNS).issubset(set(s)):
                columns = s
            else:
                record = dict(zip(columns, s))
                if check_record(record):
                    records.append(update_record(record))


    with open(outputs.get('output1'), 'w') as f:
        json.dump(records, f, indent=4, ensure_ascii=False)


if __name__ == '__main__':
    main()
