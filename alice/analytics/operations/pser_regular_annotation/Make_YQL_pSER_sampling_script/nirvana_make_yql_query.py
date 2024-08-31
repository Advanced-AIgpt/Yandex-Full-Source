import argparse
import sys
import os
import datetime
import json

SIZE_CONST = 1000


def make_script(config):
    conditions = []
    for field in config['conditions'].iterkeys():
        if type(config['conditions'][field]) == list:
            regexp = ' \"(' + '|'.join([x for x in config['conditions'][field]]) + ')\" '
            conditions += ['%s REGEXP %s' % (field, regexp)]
        else:
            conditions += ['%s REGEXP %s' % (field, config['conditions'][field])]
    if "size" in config.keys():
        limit = int(config["size"])
    else:
        limit = SIZE_CONST

    return ' AND '.join(conditions), str(limit)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-c', '--config', dest='config', help='json config file for sampling')
    parser.add_argument('-y', '--yql', dest='yql', help='yql script to execute')
    parser.add_argument('-o', '--output', dest='output', help='output table to save table with links')
    # parser.add_argument('--global_count', dest='global_count', help='output table to save total count')
    parser.add_argument('-d', '--delta', dest='delta', default=1, help='time delta for a table')
    context = parser.parse_args(sys.argv[1:])
    date = (datetime.datetime.now() - datetime.timedelta(int(context.delta))).strftime('%Y-%m-%d')

    config = json.loads(open(context.config).read())

    table = '//home/voice/vins/logs/dialogs/%s' % date
    conditions, limit = make_script(config)
    out_table = "tmp/pSER-monitoring-" + datetime.datetime.now().strftime('%Y-%m-%d-%H-%M-%S')
    # count_table = out_table + "_global_count"

    request = """
PRAGMA yt.InferSchema;
PRAGMA SimpleColumns;
USE hahn;


$sample = (
    SELECT voice_text, SOME(user_shown_text) AS user_shown_text,
    SOME(request_id) AS request_id,
    COUNT(*) AS sample_count FROM (
        SELECT Yson::ConvertToString(response.voice_text) AS voice_text,
               Yson::ConvertToString(response.cards{{0}}.text) AS user_shown_text,
               Yson::ConvertToString(request.request_id) AS request_id,
               uuid
        FROM [{table}]
        WHERE {conditions}
        AND uuid NOT LIKE "%ffffffffffffff%" 
        AND Yson::ConvertToString(response.voice_text) IS NOT NULL
        AND uuid IS NOT NULL
        AND form_name NOT REGEXP("^personal_assistant.scenarios.market")
        AND (
            form_name NOT REGEXP("^personal_assistant.scenarios.external_skill") OR
            Yson::ConvertToString(
                Yson::SerializeText(
                    ListFilter(
                        Yson::ConvertToList(form.slots), 
                        ($x) -> {{ RETURN Yson::LookupString($x, "slot") == "skill_id"; }}
                    ){{0}}
                ).value
            ) == "bd7c3799-5947-41d0-b3d3-4a35de977111"
        )
        ORDER BY RANDOM(*) LIMIT {limit}
    ) 
    GROUP BY voice_text
);


$phrase_count=(
    SELECT voice_text, 
        COUNT(*) AS count FROM ( 
        SELECT Yson::ConvertToString(response.voice_text) AS voice_text,
               uuid
        FROM [{table}]
        WHERE {conditions} 
    ) WHERE uuid NOT LIKE "%ffffffffffffff%" 
    AND voice_text IS NOT NULL
    AND uuid IS NOT NULL
    GROUP BY voice_text
);

INSERT INTO [{out_table}] WITH TRUNCATE
SELECT s.voice_text AS voice_text, user_shown_text, request_id, sample_count, count
FROM  $sample AS s LEFT JOIN $phrase_count AS p ON s.voice_text == p.voice_text;
    """.format(table=table, conditions=conditions, out_table=out_table, limit=limit)

    with open(context.output, 'w') as g:
        g.write("//" + out_table)
    with open(context.yql, 'w') as g:
        g.write(request)


if __name__ == '__main__':
    main()
