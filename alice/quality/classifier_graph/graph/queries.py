
def gen_casts(scenarios: list, all_scenarios: list):
    lines = []
    for scenario in all_scenarios:
        if scenario in scenarios:
            lines.append(f'        CAST({scenario} as STRING),')
        else:
            lines.append('        CAST(\'_\' as STRING),')
    return '\n'.join(lines)


def data_preparation_cast_scenarios_query(scenarios: list, all_scenarios: list):
    return f'''\
PRAGMA yt.InferSchema = '100';
INSERT INTO {{{{output1}}}} WITH TRUNCATE
SELECT
    Cast(TableRecordIndex() as string) as key,
    ListConcat(AsList(
{gen_casts(scenarios, all_scenarios)}
        features), '\t') as value,
forced_confident as forced_confident,
text as request_text
FROM {{{{input1}}}};
'''


def measure_quality_test_cast_scenarios_query(scenarios: list, all_scenarios: list):
    return f'''\
PRAGMA yt.InferSchema = '100';
INSERT INTO {{{{output1}}}} WITH TRUNCATE
SELECT
    Cast(TableRecordIndex() as string) as key,
    ListConcat(AsList(
{gen_casts(scenarios, all_scenarios)}
        features
    ), '	') as value,
forced_confident as forced_confident,
reqid,
text as utterance
FROM {{{{input1}}}}
ORDER BY utterance, reqid;'''
