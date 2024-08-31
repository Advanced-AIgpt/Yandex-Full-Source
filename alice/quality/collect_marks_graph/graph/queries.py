def reqs_marks_query():
    return '''\
INSERT INTO {{output1}}
SELECT reqs.*
FROM {{input1}} as reqs
LEFT ONLY JOIN {{input2}} as marks
ON reqs.session_id == marks.session_id'''


def tabify(lines):
    return map(lambda s: ' ' * 4 + s, lines)


def join_all_query(scenarios):
    def gen_select():
        lines = []
        for scenario in scenarios:
            columns = {
                f'{scenario}.mark': f'{scenario}_mark',
                f'{scenario}.metric_integral': f'{scenario}_metric'
            }
            if scenario == 'vins':
                columns = {**columns,
                           'vins.asr_text': 'asr_text',
                           'vins.intent': 'vins_intent'}
            for col_left, col_right in columns.items():
                lines.append(f'{col_left} as {col_right},')
        return '\n'.join(tabify(lines))

    def gen_from():
        lines = []
        for scenario in scenarios:
            lines.append(f'INNER JOIN {{{{input1.{scenario}}}}} as {scenario}')
            lines.append(f'ON base.request_id = {scenario}.req_id')
        return '\n'.join(tabify(lines))

    return f'''\
PRAGMA yt.InferSchema = "1000";
INSERT INTO {{{{output1}}}}
SELECT
{gen_select()}
    base.request_id as reqid, base.text as text,
    base.vins_intent as base_vins_intent,
    base.toloka_intent as toloka_intent,
    base.device_state as device_state,
    base.session_id as session_id
FROM {{{{param[requests]}}}} as base
{gen_from()}'''


def insert_marks_query(scenarios):
    def gen_select():
        lines = []
        for scenario in scenarios:
            columns = {f'{scenario}_mark', f'{scenario}_metric'}
            if scenario == 'vins':
                columns = {*columns, 'asr_text', 'vins_intent'}
            for col in columns:
                lines.append(f'UNWRAP(new_mark.{col} ?? old_mark.{col}) as {col},')
        return '\n'.join(tabify(lines))

    return f'''\
PRAGMA yt.InferSchema = '100';
$q1 = SELECT
{gen_select()}
    UNWRAP(new_mark.text ?? old_mark.text) as text,
    UNWRAP(new_mark.session_id ?? old_mark.session_id) as session_id,
    UNWRAP(new_mark.toloka_intent ?? old_mark.toloka_intent) as toloka_intent,
    UNWRAP(new_mark.reqid ?? old_mark.reqid) as reqid,
    UNWRAP(new_mark.base_vins_intent ?? old_mark.base_vins_intent) as base_vins_intent,
    UNWRAP(new_mark.device_state ?? old_mark.device_state) as device_state,
    UNWRAP(IF(new_mark.reqid IS NOT NULL, CurrentUtcTimestamp(), old_mark.evaluation_date))
    as evaluation_date
FROM {{{{input1}}}} as new_mark
FULL JOIN
{{{{param[marks]}}}} as old_mark
ON new_mark.reqid == old_mark.reqid;

$validate_schema = @@
import cyson

def validate_schema(old_marks, new_marks):
    for column in old_marks:
        if column not in new_marks:
            return False
    return True

validate_schema._yql_convert_yson = (cyson.loads, cyson.dumps)
@@;

$validate_schema = Python2::validate_schema(ParseType("(Yson?, Yson?)->Bool"), $validate_schema);

$q3 = SELECT TableRow() from (Select * WITHOUT evaluation_date from {{{{param[marks]}}}} LIMIT 1);
DISCARD SELECT
Ensure(
    Unwrap(TableRow()),
    $validate_schema(Yson::From(Unwrap($q3)), Yson::From(Unwrap(TableRow()))),
    "Mark tables have different schemas. Check scenario config."
) as value
FROM (Select * WITHOUT evaluation_date from $q1 LIMIT 1);
COMMIT;
INSERT INTO {{{{param[marks]}}}} WITH TRUNCATE
SELECT *
FROM $q1;'''
