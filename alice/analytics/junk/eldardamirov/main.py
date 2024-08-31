import json
import nirvana.job_context as nv

import datetime


ctx = nv.context()
parameters = json.loads(ctx.get_parameters().get('kwargs', '{}'))
inputs = ctx.get_inputs()
outputs = ctx.get_outputs()


def parse_params():
    """
    Parses Nirvana Global parameters,
    passed as param1 (base_date), param2 (delta)
    """
    log_start_ts = 1619211600
    base_date_ts = int(parameters.get("base_date", log_start_ts))
    base_date_ts /= 1000
    base_date_res = datetime.datetime.fromtimestamp(base_date_ts)

    delta_res = int(parameters.get("delta", 1))

    return base_date_res, delta_res


def get_neccessary_tables(tables, base_date, delta):
    """
    return ids of tables that were created in the
    following time span: [-inf, *base_date* - *delta*]
    """

    upper_limit = base_date - datetime.timedelta(days=delta)
    proper_table_urls = []

    ts_fmt = '%Y-%m-%dT%H:%M:%S.%fZ'
    for table in tables:
        create_date_attr = table["metadata"]["creation_time"]
        cur_create_date = datetime.datetime.strptime(create_date_attr, ts_fmt)

        if cur_create_date <= upper_limit:
            proper_table_urls.append({"table_id" : table["table"]})

    return proper_table_urls


def main():
    with open(inputs.get("input1"), "r") as f:
        tables = json.load(f)

    base_date, delta = parse_params()

    with open(outputs.get('output1'), 'w') as output1:
        json.dump(get_neccessary_tables(tables, base_date, delta), output1, ensure_ascii=False)


if __name__ == '__main__':
    main()

